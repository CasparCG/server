/*
 * Copyright (c) 2026
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 */

// macOS screen consumer: native NSWindow + CAMetalLayer.
//
// AppKit requires windows to be created and manipulated on the main thread. On macOS the
// CasparCG shell runs the Cocoa run loop on the main thread (see shell/macos_main.mm), so
// window management is dispatched to the main queue while rendering happens on the consumer
// thread (CAMetalLayer drawables may be acquired and presented from any thread).

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "screen_config.h"
#include "screen_consumer.h"

#include "consumer_screen_metal.h"

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/future.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/os/thread.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/geometry.h>
#include <core/monitor/monitor.h>
#include <core/video_format.h>

#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

// Shared between the main thread (AppKit callbacks) and the render thread.
struct caspar_screen_window_state
{
    std::atomic<bool> closed{false};
    std::atomic<bool> resized{true};
    std::atomic<int>  pixel_width{0};
    std::atomic<int>  pixel_height{0};
};

@interface CasparScreenView : NSView
@end

@implementation CasparScreenView
- (CALayer*)makeBackingLayer
{
    return [CAMetalLayer layer];
}
- (BOOL)wantsUpdateLayer
{
    return YES;
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}
@end

@interface CasparScreenWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) caspar_screen_window_state* state;
- (void)updateSize:(NSWindow*)window;
@end

@implementation CasparScreenWindowDelegate
- (void)updateSize:(NSWindow*)window
{
    NSView* view  = window.contentView;
    NSSize  size  = [view convertSizeToBacking:view.bounds.size];
    CAMetalLayer* layer = (CAMetalLayer*)view.layer;
    layer.contentsScale = window.backingScaleFactor;
    _state->pixel_width  = std::max(1, static_cast<int>(size.width));
    _state->pixel_height = std::max(1, static_cast<int>(size.height));
    _state->resized      = true;
}
- (void)windowDidResize:(NSNotification*)notification
{
    [self updateSize:notification.object];
}
- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
    [self updateSize:notification.object];
}
- (void)windowDidChangeScreen:(NSNotification*)notification
{
    [self updateSize:notification.object];
}
- (void)windowWillClose:(NSNotification*)notification
{
    _state->closed = true;
}
@end

namespace caspar { namespace screen {

namespace {

void run_on_main_thread(void (^block)(void))
{
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

struct metal_vertex
{
    float position[4];
    float tex_coord[4];
};

struct metal_uniforms
{
    int32_t key_only;
    int32_t colour_space;
    int32_t window_width;
    int32_t padding;
};

constexpr int IN_FLIGHT = 3;

} // namespace

struct screen_consumer
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    int screen_width_  = format_desc_.width;
    int screen_height_ = format_desc_.height;
    int square_width_  = format_desc_.square_width;
    int square_height_ = format_desc_.square_height;

    std::vector<core::frame_geometry::coord> draw_coords_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_;

    caspar_screen_window_state  state_;
    NSWindow*                   window_   = nil;
    CasparScreenWindowDelegate* delegate_ = nil;
    CAMetalLayer*               layer_    = nil;

    id<MTLDevice>              device_   = nil;
    id<MTLCommandQueue>        queue_    = nil;
    id<MTLRenderPipelineState> pipeline_ = nil;
    id<MTLSamplerState>        sampler_  = nil;
    std::vector<id<MTLTexture>> textures_;
    dispatch_semaphore_t        in_flight_ = nil;
    int                         texture_index_ = 0;

    std::atomic<bool> is_running_{true};
    std::thread       thread_;

    screen_consumer(const screen_consumer&)            = delete;
    screen_consumer& operator=(const screen_consumer&) = delete;

    screen_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
    {
        if (config_.gpu_texture) {
            CASPAR_LOG(warning) << print() << " gpu-texture is not supported on macOS, using host frames.";
        }

        if (format_desc_.format == core::video_format::ntsc &&
            config_.aspect == configuration::aspect_ratio::aspect_4_3) {
            // Use default values which are 4:3.
        } else {
            if (config_.aspect == configuration::aspect_ratio::aspect_16_9) {
                square_width_ = format_desc.height * 16 / 9;
            } else if (config_.aspect == configuration::aspect_ratio::aspect_4_3) {
                square_width_ = format_desc.height * 4 / 3;
            }
        }

        auto delay_frames = config_.delay.in_frames(format_desc_.fps);
        delay_frames      = std::clamp<int>(delay_frames, 0, format_desc_.fps);
        CASPAR_LOG(info) << print() << " Latency: " << delay_frames << " frames";

        frame_buffer_.set_capacity(delay_frames + 1);
        while (delay_frames--)
            frame_buffer_.push(core::const_frame{});

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        init_metal();
        create_window();

        thread_ = std::thread([this] {
            set_thread_name(L"screen-metal");
            try {
                while (is_running_) {
                    @autoreleasepool {
                        tick();
                    }
                }
            } catch (tbb::user_abort&) {
                // Do nothing
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                is_running_ = false;
            }
        });
    }

    ~screen_consumer()
    {
        is_running_ = false;
        frame_buffer_.abort();
        if (thread_.joinable()) {
            thread_.join();
        }

        // Wait for in-flight GPU work before releasing resources.
        if (in_flight_) {
            for (int n = 0; n < IN_FLIGHT; ++n)
                dispatch_semaphore_wait(in_flight_, DISPATCH_TIME_FOREVER);
            for (int n = 0; n < IN_FLIGHT; ++n)
                dispatch_semaphore_signal(in_flight_);
        }

        NSWindow*                   window   = window_;
        CasparScreenWindowDelegate* delegate = delegate_;
        window_                              = nil;
        delegate_                            = nil;
        layer_                               = nil;
        run_on_main_thread(^{
          window.delegate = nil;
          [window orderOut:nil];
          [window close];
          (void)delegate;
        });
    }

    void init_metal()
    {
        device_ = MTLCreateSystemDefaultDevice();
        if (!device_) {
            CASPAR_THROW_EXCEPTION(not_supported() << msg_info("No Metal device available."));
        }
        queue_ = [device_ newCommandQueue];

        NSError*  error  = nil;
        NSString* source = [NSString stringWithUTF8String:reinterpret_cast<const char*>(metal_shader)];
        id<MTLLibrary> library = [device_ newLibraryWithSource:source options:nil error:&error];
        if (!library) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(std::string("Failed to compile screen shader: ") +
                                                                  error.localizedDescription.UTF8String));
        }

        MTLRenderPipelineDescriptor* desc     = [[MTLRenderPipelineDescriptor alloc] init];
        desc.vertexFunction                   = [library newFunctionWithName:@"screen_vertex"];
        desc.fragmentFunction                 = [library newFunctionWithName:@"screen_fragment"];
        desc.colorAttachments[0].pixelFormat  = MTLPixelFormatBGRA8Unorm;
        pipeline_ = [device_ newRenderPipelineStateWithDescriptor:desc error:&error];
        if (!pipeline_) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(std::string("Failed to create screen pipeline: ") +
                                                                  error.localizedDescription.UTF8String));
        }

        const bool nearest = config_.colour_space == configuration::colour_spaces::datavideo_full ||
                             config_.colour_space == configuration::colour_spaces::datavideo_limited;
        MTLSamplerDescriptor* sdesc = [[MTLSamplerDescriptor alloc] init];
        sdesc.minFilter             = nearest ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
        sdesc.magFilter             = sdesc.minFilter;
        sdesc.sAddressMode          = MTLSamplerAddressModeClampToEdge;
        sdesc.tAddressMode          = MTLSamplerAddressModeClampToEdge;
        sampler_                    = [device_ newSamplerStateWithDescriptor:sdesc];

        // Frames are BGRA, 8 bit or 16 bit per channel.
        MTLTextureDescriptor* tdesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:config_.high_bitdepth ? MTLPixelFormatRGBA16Unorm
                                                                     : MTLPixelFormatBGRA8Unorm
                                         width:format_desc_.width
                                        height:format_desc_.height
                                     mipmapped:NO];
        tdesc.usage       = MTLTextureUsageShaderRead;
        tdesc.storageMode = MTLStorageModeShared;
        if (config_.high_bitdepth) {
            tdesc.swizzle = MTLTextureSwizzleChannelsMake(
                MTLTextureSwizzleBlue, MTLTextureSwizzleGreen, MTLTextureSwizzleRed, MTLTextureSwizzleAlpha);
        }
        for (int n = 0; n < IN_FLIGHT; ++n) {
            textures_.push_back([device_ newTextureWithDescriptor:tdesc]);
        }
        in_flight_ = dispatch_semaphore_create(IN_FLIGHT);

        CASPAR_LOG(info) << print() << " Using Metal device: " << device_.name.UTF8String;
    }

    void create_window()
    {
        __block NSWindow*                   window   = nil;
        __block CasparScreenWindowDelegate* delegate = nil;
        __block CAMetalLayer*               layer    = nil;

        const auto title = u8(print());

        run_on_main_thread(^{
          [NSApplication sharedApplication];

          NSArray<NSScreen*>* screens = [NSScreen screens];
          NSScreen*           screen  = [NSScreen mainScreen];
          if (config_.screen_index >= 0 && config_.screen_index < static_cast<int>(screens.count)) {
              screen = screens[config_.screen_index];
          } else if (config_.screen_index != 0) {
              CASPAR_LOG(warning) << print() << L" Invalid screen-index: " << config_.screen_index;
          }

          const NSRect screen_frame = screen.frame;
          NSRect       frame;
          NSWindowStyleMask style;

          if (config_.windowed) {
              const int    content_square_width = config_.sbs_key ? square_width_ * 2 : square_width_;
              double width  = 0;
              double height = 0;
              if (config_.screen_width > 0 && config_.screen_height > 0) {
                  width  = config_.screen_width;
                  height = config_.screen_height;
              } else if (config_.screen_width > 0) {
                  width  = config_.screen_width;
                  height = static_cast<double>(square_height_) * config_.screen_width / content_square_width;
              } else if (config_.screen_height > 0) {
                  height = config_.screen_height;
                  width  = static_cast<double>(content_square_width) * config_.screen_height / square_height_;
              } else {
                  // Default: native size in points, scaled down to fit the visible screen area.
                  const NSRect visible = screen.visibleFrame;
                  width                = content_square_width;
                  height               = square_height_;
                  const double scale =
                      std::min({1.0, visible.size.width * 0.8 / width, visible.size.height * 0.8 / height});
                  width *= scale;
                  height *= scale;
              }

              style = config_.borderless ? NSWindowStyleMaskBorderless
                                         : (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);

              // Config coordinates are top-left based, Cocoa is bottom-left based.
              const NSRect content = NSMakeRect(0, 0, width, height);
              frame                = [NSWindow frameRectForContentRect:content styleMask:style];
              frame.origin.x       = screen_frame.origin.x + config_.screen_x;
              frame.origin.y =
                  screen_frame.origin.y + screen_frame.size.height - config_.screen_y - frame.size.height;
              if (config_.screen_x == 0 && config_.screen_y == 0 && !config_.borderless) {
                  // Keep the title bar below the menu bar.
                  const NSRect visible = screen.visibleFrame;
                  frame.origin.y       = visible.origin.y + visible.size.height - frame.size.height;
              }
          } else {
              style = NSWindowStyleMaskBorderless;
              frame = screen_frame;
          }

          window = [[NSWindow alloc] initWithContentRect:[NSWindow contentRectForFrameRect:frame styleMask:style]
                                               styleMask:style
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO
                                                  screen:screen];
          window.releasedWhenClosed = NO;
          window.title              = [NSString stringWithUTF8String:title.c_str()];
          window.backgroundColor    = NSColor.blackColor;
          [window setFrame:frame display:NO];

          CasparScreenView* view = [[CasparScreenView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
          view.wantsLayer        = YES;
          window.contentView     = view;

          layer                 = (CAMetalLayer*)view.layer;
          layer.device          = device_;
          layer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
          layer.framebufferOnly = YES;
          layer.opaque          = YES;
          layer.displaySyncEnabled = config_.vsync ? YES : NO;
          layer.maximumDrawableCount = 3;

          if (!config_.windowed) {
              window.level              = NSMainMenuWindowLevel + 1;
              window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                          NSWindowCollectionBehaviorStationary |
                                          NSWindowCollectionBehaviorFullScreenAuxiliary;
          } else if (config_.always_on_top) {
              window.level = NSFloatingWindowLevel;
          }

          delegate        = [[CasparScreenWindowDelegate alloc] init];
          delegate.state  = &state_;
          window.delegate = delegate;
          [delegate updateSize:window];

          [window orderFrontRegardless];
          if (config_.windowed) {
              [window makeKeyWindow];
          }

          if (!config_.interactive) {
              [NSCursor hide];
          }
        });

        window_   = window;
        delegate_ = delegate;
        layer_    = layer;

        if (config_.vsync) {
            CASPAR_LOG(info) << print() << " Enabled vsync.";
        }
        if (config_.colour_space == configuration::colour_spaces::datavideo_full ||
            config_.colour_space == configuration::colour_spaces::datavideo_limited) {
            CASPAR_LOG(info) << print() << " Enabled colours conversion for DataVideo TC-100/TC-200 "
                             << (config_.colour_space == configuration::colour_spaces::datavideo_full
                                     ? "(Full Range)."
                                     : "(Limited Range).");
        }
    }

    void tick()
    {
        if (state_.closed) {
            is_running_ = false;
            return;
        }

        core::const_frame in_frame;
        while (!frame_buffer_.try_pop(in_frame)) {
            if (!is_running_ || state_.closed) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (!in_frame) {
            return;
        }

        if (state_.resized.exchange(false)) {
            screen_width_  = state_.pixel_width;
            screen_height_ = state_.pixel_height;
            layer_.drawableSize = CGSizeMake(screen_width_, screen_height_);
            calculate_aspect();
        }

        dispatch_semaphore_wait(in_flight_, DISPATCH_TIME_FOREVER);

        // Upload
        id<MTLTexture> texture = textures_[texture_index_];
        texture_index_         = (texture_index_ + 1) % IN_FLIGHT;

        const auto  bytes_per_pixel = config_.high_bitdepth ? 8 : 4;
        const auto& image           = in_frame.image_data(0);
        if (image.size() >= static_cast<std::size_t>(format_desc_.width * format_desc_.height * bytes_per_pixel)) {
            [texture replaceRegion:MTLRegionMake2D(0, 0, format_desc_.width, format_desc_.height)
                       mipmapLevel:0
                         withBytes:image.begin()
                       bytesPerRow:format_desc_.width * bytes_per_pixel];
        }

        // Display
        id<CAMetalDrawable> drawable = [layer_ nextDrawable];
        if (!drawable) {
            dispatch_semaphore_signal(in_flight_);
            return;
        }

        MTLRenderPassDescriptor* pass        = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture     = drawable.texture;
        pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor  = MTLClearColorMake(0, 0, 0, 1);

        id<MTLCommandBuffer>        cmd     = [queue_ commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [cmd renderCommandEncoderWithDescriptor:pass];

        std::vector<metal_vertex> vertices;
        vertices.reserve(draw_coords_.size());
        for (auto& c : draw_coords_) {
            vertices.push_back({{static_cast<float>(c.vertex_x), static_cast<float>(c.vertex_y), 0.0f, 1.0f},
                                {static_cast<float>(c.texture_x),
                                 static_cast<float>(c.texture_y),
                                 static_cast<float>(c.texture_r),
                                 static_cast<float>(c.texture_q)}});
        }

        [encoder setRenderPipelineState:pipeline_];
        [encoder setVertexBytes:vertices.data() length:vertices.size() * sizeof(metal_vertex) atIndex:0];
        [encoder setFragmentTexture:texture atIndex:0];
        [encoder setFragmentSamplerState:sampler_ atIndex:0];

        metal_uniforms uniforms{};
        uniforms.colour_space = static_cast<int32_t>(config_.colour_space);
        uniforms.window_width = screen_width_;

        if (config_.sbs_key) {
            const auto half = static_cast<NSUInteger>(vertices.size() / 2);

            uniforms.key_only = 0;
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:half];

            uniforms.key_only = 1;
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:half vertexCount:half];
        } else {
            uniforms.key_only = config_.key_only ? 1 : 0;
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertices.size()];
        }

        [encoder endEncoding];
        [cmd presentDrawable:drawable];

        dispatch_semaphore_t in_flight = in_flight_;
        [cmd addCompletedHandler:^(id<MTLCommandBuffer>) {
          dispatch_semaphore_signal(in_flight);
        }];
        [cmd commit];

        graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
        tick_timer_.restart();
    }

    std::future<bool> send(core::video_field field, const core::const_frame& frame)
    {
        const int MAX_TRIES = 3;
        int       count     = 0;
        while (count++ < MAX_TRIES && is_running_) {
            if (frame_buffer_.try_push(frame))
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (count == MAX_TRIES) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
        }

        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print() const { return config_.name + L" " + channel_and_format(); }

    void calculate_aspect()
    {
        std::pair<float, float> target_ratio = none();
        if (config_.stretch == screen::stretch::fill) {
            target_ratio = Fill();
        } else if (config_.stretch == screen::stretch::uniform) {
            target_ratio = uniform();
        } else if (config_.stretch == screen::stretch::uniform_to_fill) {
            target_ratio = uniform_to_fill();
        }

        if (config_.sbs_key) {
            draw_coords_ = {
                // First half fill
                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {0, target_ratio.second, 1.0, 0.0},                   // upper right
                {0, -target_ratio.second, 1.0, 1.0},                  // lower right

                {-target_ratio.first, target_ratio.second, 0.0, 0.0},  // upper left
                {0, -target_ratio.second, 1.0, 1.0},                   // lower right
                {-target_ratio.first, -target_ratio.second, 0.0, 1.0}, // lower left

                // Second half key
                {0, target_ratio.second, 0.0, 0.0},                   // upper left
                {target_ratio.first, target_ratio.second, 1.0, 0.0},  // upper right
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right

                {0, target_ratio.second, 0.0, 0.0},                   // upper left
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right
                {0, -target_ratio.second, 0.0, 1.0}                   // lower left
            };
        } else {
            draw_coords_ = {
                //    vertex    texture
                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {target_ratio.first, target_ratio.second, 1.0, 0.0},  // upper right
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right

                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right
                {-target_ratio.first, -target_ratio.second, 0.0, 1.0} // lower left
            };
        }
    }

    std::pair<float, float> none() const
    {
        float width =
            static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) / static_cast<float>(screen_width_);
        float height = static_cast<float>(square_height_) / static_cast<float>(screen_height_);

        return std::make_pair(width, height);
    }

    std::pair<float, float> uniform() const
    {
        float aspect = static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) /
                       static_cast<float>(square_height_);
        float width  = std::min(1.0f, static_cast<float>(screen_height_) * aspect / static_cast<float>(screen_width_));
        float height = static_cast<float>(screen_width_ * width) / static_cast<float>(screen_height_ * aspect);

        return std::make_pair(width, height);
    }

    static std::pair<float, float> Fill() { return std::make_pair(1.0f, 1.0f); }

    std::pair<float, float> uniform_to_fill() const
    {
        float wr =
            static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) / static_cast<float>(screen_width_);
        float hr    = static_cast<float>(square_height_) / static_cast<float>(screen_height_);
        float r_inv = 1.0f / std::min(wr, hr);

        float width  = wr * r_inv;
        float height = hr * r_inv;

        return std::make_pair(width, height);
    }
};

struct screen_consumer_proxy : public core::frame_consumer
{
    const configuration              config_;
    std::unique_ptr<screen_consumer> consumer_;

  public:
    explicit screen_consumer_proxy(configuration config)
        : config_(std::move(config))
    {
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        consumer_.reset();
        consumer_ = std::make_unique<screen_consumer>(config_, format_desc, channel_info.index);
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        return consumer_->send(field, frame);
    }

    std::wstring print() const override { return consumer_ ? consumer_->print() : L"[screen_consumer]"; }

    std::wstring name() const override { return L"screen"; }

    bool has_synchronization_clock() const override { return false; }

    int index() const override { return 600 + (config_.key_only ? 10 : 0) + config_.screen_index; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["screen/name"]          = config_.name;
        state["screen/index"]         = config_.screen_index;
        state["screen/key_only"]      = config_.key_only;
        state["screen/always_on_top"] = config_.always_on_top;
        return state;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    auto config = parse_params(params, channel_info);
    if (!config) {
        return core::frame_consumer::empty();
    }
    return spl::make_shared<screen_consumer_proxy>(*config);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    return spl::make_shared<screen_consumer_proxy>(parse_ptree(ptree, channel_info));
}

}} // namespace caspar::screen
