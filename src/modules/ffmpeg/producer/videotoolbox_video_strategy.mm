/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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
 *
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

// The one place the Metal interop structs are needed; every other Vulkan translation unit is
// platform-neutral, so the define stays local to this file.
#define VK_USE_PLATFORM_METAL_EXT

#include "video_strategy.h"

#include "vulkan_frame_import.h"

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <common/env.h>
#include <common/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#import <Metal/Metal.h>
#include <CoreVideo/CoreVideo.h>
#include <IOSurface/IOSurfaceRef.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace caspar { namespace ffmpeg {

namespace {

/// VideoToolbox's deinterlacer. Metal compute rather than bwdif — FFmpeg has no
/// bwdif_videotoolbox, so interlaced material on macOS is deinterlaced to a different standard
/// than everywhere else. Deliberate: yadif on the hardware beats a software decode.
constexpr const char* deinterlacer = "yadif_videotoolbox";

using buffer_ref = std::shared_ptr<AVBufferRef>;

buffer_ref wrap_buffer_ref(AVBufferRef* ref)
{
    return buffer_ref(ref, [](AVBufferRef* p) { av_buffer_unref(&p); });
}

std::wstring error_text(int ret)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, buf, sizeof(buf));
    return u16(std::string(buf));
}

AVPixelFormat select_videotoolbox_format(AVCodecContext* ctx, const AVPixelFormat* formats)
{
    for (auto p = formats; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == AV_PIX_FMT_VIDEOTOOLBOX)
            return *p;
    }
    // No VideoToolbox hwaccel for this stream after all — let the decoder pick its usual
    // software format; the producer notices the missing frames context and moves on.
    return avcodec_default_get_format(ctx, formats);
}

/// A mixer plane's sample layout, as the two APIs that have to agree on it spell the same thing.
/// The mixer's desc is the single source of truth: a plane is `stride` components of `depth`
/// bits, which is exactly what one texture of an IOSurface plane must be.
struct plane_format
{
    vk::Format     vulkan;
    MTLPixelFormat metal;
};

bool format_of_plane(const core::pixel_format_desc::plane& plane, plane_format& out)
{
    const bool sixteen = plane.depth != common::bit_depth::bit8;

    switch (plane.stride) {
        case 1:
            out = sixteen ? plane_format{vk::Format::eR16Unorm, MTLPixelFormatR16Unorm}
                          : plane_format{vk::Format::eR8Unorm, MTLPixelFormatR8Unorm};
            return true;
        case 2:
            out = sixteen ? plane_format{vk::Format::eR16G16Unorm, MTLPixelFormatRG16Unorm}
                          : plane_format{vk::Format::eR8G8Unorm, MTLPixelFormatRG8Unorm};
            return true;
        default:
            // Nothing VideoToolbox emits reaches here: mixer_layout() only admits a format whose
            // plane count matches the decoder's, which rules out the packed ones (uyvy422, the
            // 4444 family) before this is asked.
            return false;
    }
}

} // namespace

// ---------------------------------------------------------------------------------------

/// Decode with VideoToolbox, then let the mixer read the decoder's own frames.
///
/// The whole point is what does NOT happen: a VideoToolbox frame is a CVPixelBuffer backed by an
/// IOSurface, and each of its planes can back a Metal texture, which in turn can back a VkImage
/// through VK_EXT_metal_objects. So the decoder's output IS the source image — nothing is copied
/// to the host and nothing is copied across an API boundary; the only copy is the same
/// device-local blit into mixer textures that every hardware strategy ends with.
///
/// Two limits of MoltenVK shape this (verified on an M2 Pro, MoltenVK 1.4):
///
///   - Importing a planar IOSurface as one multi-planar VkImage does not work. MoltenVK routes
///     VkImportMetalIOSurfaceInfoEXT through its legacy vkUseIOSurfaceMVK, which compares the
///     whole surface's bytes-per-element against the image's block size and rejects the pair.
///     Hence one VkImage per plane, each backed by its own MTLTexture — which is what the mixer
///     wants anyway, one texture per plane.
///   - An imported image arrives in VK_IMAGE_LAYOUT_UNDEFINED, and transitioning out of that is
///     content-discarding per spec. It is not on MoltenVK, where layouts are near no-ops and the
///     memory is the IOSurface itself. This rests on that.
///
/// There is no synchronisation to do: VideoToolbox has finished with a frame before FFmpeg hands
/// it over, so unlike the Vulkan and CUDA strategies there is no timeline to wait or signal. What
/// must be respected instead is the other direction — the decoder may not recycle the surface
/// while our copy is still reading it — which is why the frame is handed to the import as a
/// keep-alive rather than released when make_frame returns.
class videotoolbox_video_strategy : public video_strategy
{
    const std::unique_ptr<vulkan_frame_import> import_;
    const buffer_ref                           device_;
    id<MTLDevice>                              metal_device_;
    const vk::Device                           vk_device_;

    /// One imported frame surface. VideoToolbox recycles a small pool of IOSurfaces for the whole
    /// of playback, so these are built once each and then simply looked up — without the cache
    /// this would create and destroy a VkImage per plane per frame.
    struct imported_surface
    {
        IOSurfaceRef                surface = nullptr; // retained
        std::vector<id<MTLTexture>> textures;          // retained
        std::vector<vk::Image>      images;            // ours to destroy
    };
    std::unordered_map<IOSurfaceRef, imported_surface> surfaces_;

    bool warned_unsupported_format_ = false;
    bool warned_import_failed_      = false;

  public:
    videotoolbox_video_strategy(std::unique_ptr<vulkan_frame_import> import,
                                buffer_ref                           device,
                                id<MTLDevice>                        metal_device)
        : import_(std::move(import))
        , device_(std::move(device))
        , metal_device_([metal_device retain])
        , vk_device_(import_->vk_device())
    {
    }

    ~videotoolbox_video_strategy() override
    {
        // Nothing may still be reading the images we are about to destroy.
        import_->drain();

        for (auto& [surface, entry] : surfaces_) {
            for (auto image : entry.images)
                vk_device_.destroyImage(image);
            for (id<MTLTexture> texture : entry.textures)
                [texture release];
            CFRelease(entry.surface);
        }
        [metal_device_ release];
    }

    std::wstring name() const override { return L"videotoolbox"; }

    bool open_decoder(AVCodecContext& ctx, const AVCodec& codec) override
    {
        if (!supports_videotoolbox(codec))
            return false;

        ctx.hw_device_ctx = av_buffer_ref(device_.get());
        if (!ctx.hw_device_ctx)
            return false;

        ctx.get_format = select_videotoolbox_format;
        return true;
    }

    AVBufferRef* hw_device_context() const override { return device_.get(); }

    bool accepts(const AVFrame& frame) const override
    {
        // A software frame means get_format() declined VideoToolbox after all — either the codec
        // has no hwaccel for this stream, or the hwaccel failed to initialise and FFmpeg quietly
        // retried without it.
        if (frame.format != AV_PIX_FMT_VIDEOTOOLBOX || !frame.hw_frames_ctx)
            return false;

        const auto* frames_ctx = reinterpret_cast<const AVHWFramesContext*>(frame.hw_frames_ctx->data);
        if (!vulkan_frame_import::has_mixer_layout(frames_ctx->sw_format, frames_ctx->width, frames_ctx->height)) {
            // ProRes 4444 lands here: VideoToolbox returns it packed as ayuv64, which the mixer
            // has no plane layout for. 4:2:0 in either depth — the h264/hevc bulk — does not.
            CASPAR_LOG(info) << L"[ffmpeg] The mixer has no layout for VideoToolbox frame format "
                             << u16(av_get_pix_fmt_name(frames_ctx->sw_format))
                             << L"; this file decodes on the CPU instead.";
            return false;
        }
        return true;
    }

    std::string deinterlace_filter(const std::string& deint) const override
    {
        return (boost::format("%s=mode=send_field:parity=auto:deint=%s") % deinterlacer % deint).str();
    }

    std::vector<AVPixelFormat> sink_formats() const override { return {AV_PIX_FMT_VIDEOTOOLBOX}; }

    core::draw_frame make_frame(void*                            tag,
                                std::shared_ptr<AVFrame>         video,
                                std::shared_ptr<AVFrame>         audio,
                                core::color_space                color_space,
                                core::frame_geometry::scale_mode scale_mode) override
    {
        if (!video || video->format != AV_PIX_FMT_VIDEOTOOLBOX || !video->hw_frames_ctx || !video->data[3]) {
            return import_->host_frame(tag, std::move(video), std::move(audio), color_space, scale_mode);
        }

        const auto* frames_ctx = reinterpret_cast<const AVHWFramesContext*>(video->hw_frames_ctx->data);

        // accepts() already refused any format without a layout, so this only guards a decoder
        // that changed format mid-stream.
        const auto layout =
            vulkan_frame_import::mixer_layout(frames_ctx->sw_format, video->width, video->height, color_space);
        if (!layout) {
            if (!warned_unsupported_format_) {
                warned_unsupported_format_ = true;
                CASPAR_LOG(warning) << L"[ffmpeg] VideoToolbox frame format changed to "
                                    << u16(av_get_pix_fmt_name(frames_ctx->sw_format))
                                    << L", which the mixer has no layout for; dropping video.";
            }
            return core::draw_frame{};
        }

        const auto* entry = import_surface(reinterpret_cast<CVPixelBufferRef>(video->data[3]), *layout);

        if (!entry) {
            if (!warned_import_failed_) {
                warned_import_failed_ = true;
                CASPAR_LOG(warning) << L"[ffmpeg] Could not import a VideoToolbox frame into Vulkan; "
                                       L"dropping video.";
            }
            return core::draw_frame{};
        }

        // Every image arrives in eUndefined: Metal wrote it, and Vulkan layouts do not describe
        // what another API did. See the class note on why that keeps the contents here.
        const std::vector<vk::ImageLayout> layouts(entry->images.size(), vk::ImageLayout::eUndefined);

        return import_->import_images(tag,
                                      entry->images,
                                      layouts,
                                      *layout,
                                      {}, // VideoToolbox is done with the frame before we see it
                                      audio,
                                      scale_mode,
                                      std::shared_ptr<void>(video));
    }

  private:
    static bool supports_videotoolbox(const AVCodec& codec)
    {
        for (int i = 0;; ++i) {
            const auto* config = avcodec_get_hw_config(&codec, i);
            if (!config)
                return false;
            if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                config->device_type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX)
                return true;
        }
    }

    /// The frame's planes as VkImages, built on first sight of this IOSurface and reused for every
    /// later frame that comes back on it. Null when this surface cannot be imported at all.
    const imported_surface* import_surface(CVPixelBufferRef pixel_buffer, const core::pixel_format_desc& desc)
    {
        IOSurfaceRef surface = CVPixelBufferGetIOSurface(pixel_buffer);
        if (!surface)
            return nullptr;

        if (const auto it = surfaces_.find(surface); it != surfaces_.end())
            return &it->second;

        const auto nb_planes = desc.planes.size();
        if (CVPixelBufferGetPlaneCount(pixel_buffer) != nb_planes)
            return nullptr;

        imported_surface entry;
        CFRetain(surface);
        entry.surface = surface;

        // A texture may only be shared with the CPU the way its surface is; on Apple silicon that
        // is one memory, on an Intel Mac the discrete case has to be managed.
        const MTLStorageMode storage = metal_device_.hasUnifiedMemory ? MTLStorageModeShared : MTLStorageModeManaged;

        for (size_t plane = 0; plane < nb_planes; ++plane) {
            const auto& want = desc.planes[plane];

            plane_format format{};
            if (!format_of_plane(want, format)) {
                release(entry);
                return nullptr;
            }

            const auto width  = static_cast<int>(IOSurfaceGetWidthOfPlane(surface, plane));
            const auto height = static_cast<int>(IOSurfaceGetHeightOfPlane(surface, plane));
            if (width != want.width || height != want.height) {
                CASPAR_LOG(warning) << L"[ffmpeg] VideoToolbox plane " << plane << L" is " << width << L"x" << height
                                    << L" where the mixer expects " << want.width << L"x" << want.height
                                    << L"; not importing.";
                release(entry);
                return nullptr;
            }

            auto* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format.metal
                                                                                  width:width
                                                                                 height:height
                                                                              mipmapped:NO];
            descriptor.usage       = MTLTextureUsageShaderRead;
            descriptor.storageMode = storage;

            id<MTLTexture> texture = [metal_device_ newTextureWithDescriptor:descriptor
                                                                  iosurface:surface
                                                                      plane:plane];
            if (!texture) {
                release(entry);
                return nullptr;
            }
            entry.textures.push_back(texture); // +1 from newTextureWithDescriptor

            VkImportMetalTextureInfoEXT metal{VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT};
            // The whole of a single-plane image, not a plane of a multi-planar one.
            metal.plane      = VK_IMAGE_ASPECT_COLOR_BIT;
            metal.mtlTexture = texture;

            vk::ImageCreateInfo info;
            info.pNext         = &metal;
            info.imageType     = vk::ImageType::e2D;
            info.format        = format.vulkan;
            info.extent        = vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);
            info.mipLevels     = 1;
            info.arrayLayers   = 1;
            info.samples       = vk::SampleCountFlagBits::e1;
            info.tiling        = vk::ImageTiling::eOptimal;
            info.usage         = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;
            info.sharingMode   = vk::SharingMode::eExclusive;
            info.initialLayout = vk::ImageLayout::eUndefined;

            // No memory is bound and none may be: the image's storage IS the imported texture's.
            vk::Image image;
            if (vk_device_.createImage(&info, nullptr, &image) != vk::Result::eSuccess) {
                release(entry);
                return nullptr;
            }
            entry.images.push_back(image);
        }

        return &surfaces_.emplace(surface, std::move(entry)).first->second;
    }

    void release(imported_surface& entry)
    {
        for (auto image : entry.images)
            vk_device_.destroyImage(image);
        for (id<MTLTexture> texture : entry.textures)
            [texture release];
        if (entry.surface)
            CFRelease(entry.surface);
        entry = {};
    }
};

std::shared_ptr<video_strategy>
try_create_videotoolbox_video_strategy(const std::shared_ptr<core::frame_factory>& frame_factory)
{
    const auto deint =
        env::properties().get<std::wstring>(L"configuration.ffmpeg.producer.auto-deinterlace", L"interlaced");
    if (deint != L"none" && !avfilter_get_by_name(deinterlacer)) {
        CASPAR_LOG(debug) << L"[ffmpeg] This FFmpeg build has no " << u16(deinterlacer)
                          << L" filter, so VideoToolbox decoding cannot deinterlace; not offering it.";
        return nullptr;
    }

    // Import-only: VideoToolbox decodes into its own surfaces, so FFmpeg is never told about our
    // Vulkan device. That is what makes this work at all on macOS, where FFmpeg ships with the
    // VideoToolbox hardware device type and no Vulkan one.
    auto import = vulkan_frame_import::create_for_import(frame_factory);
    if (!import)
        return nullptr;

    // Without this extension there is no way to give a VkImage the decoder's memory, and a copy
    // through the host would be a different strategy, not this one.
    if (!import->has_device_extension(VK_EXT_METAL_OBJECTS_EXTENSION_NAME)) {
        CASPAR_LOG(debug) << L"[ffmpeg] The Vulkan device has no VK_EXT_metal_objects; "
                             L"VideoToolbox decoding unavailable.";
        return nullptr;
    }

    id<MTLDevice> metal_device = MTLCreateSystemDefaultDevice();
    if (!metal_device) {
        CASPAR_LOG(debug) << L"[ffmpeg] No Metal device; VideoToolbox decoding unavailable.";
        return nullptr;
    }

    AVBufferRef* device = nullptr;
    const auto   ret = av_hwdevice_ctx_create(&device, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0);
    if (ret < 0) {
        CASPAR_LOG(debug) << L"[ffmpeg] No VideoToolbox device (" << error_text(ret)
                          << L"); hardware decoding unavailable.";
        [metal_device release];
        return nullptr;
    }

    CASPAR_LOG(info) << L"[ffmpeg] VideoToolbox decoding available, importing into the accelerator device.";
    auto strategy = std::make_shared<videotoolbox_video_strategy>(
        std::move(import), wrap_buffer_ref(device), metal_device);

    // MTLCreateSystemDefaultDevice returned it owned, and the strategy took its own reference.
    [metal_device release];
    return strategy;
}

}} // namespace caspar::ffmpeg
