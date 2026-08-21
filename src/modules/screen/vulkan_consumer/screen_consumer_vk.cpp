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

#include "screen_consumer_vk.h"

#include "../util/config.h"
#include "util/window.h"

#include "util/render_pipeline.h"
#include "util/swapchain.h"

#include <accelerator/vulkan/util/device.h>
#include <accelerator/vulkan/util/texture.h>
#include <accelerator/vulkan/util/vulkan_queue.h>

#include <common/array.h>
#include <common/bit_depth.h>
#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/future.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <thread>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace caspar { namespace screen { namespace vulkan {

struct screen_consumer_vk
{
    const configuration                                config_;
    core::video_format_desc                            format_desc_;
    int                                                channel_index_;
    std::shared_ptr<accelerator::vulkan::device>       device_;
    std::shared_ptr<accelerator::vulkan::vulkan_queue> queue_;

    // A plain command pool for render_pipeline's per-slot buffers. The consumer
    // never goes through a command_context here: its only submit is the WSI
    // present, which must signal a binary semaphore (record_and_submit signals
    // only its timeline) — so render_pipeline submits directly under the queue
    // lock.
    vk::CommandPool command_pool_;

    int screen_width_  = format_desc_.width;
    int screen_height_ = format_desc_.height;
    int square_width_  = format_desc_.square_width;
    int square_height_ = format_desc_.square_height;
    int screen_x_      = 0;
    int screen_y_      = 0;

    std::unique_ptr<screen_window>   window_;
    std::unique_ptr<swapchain>       swapchain_;
    std::unique_ptr<render_pipeline> render_pipeline_;

    // Per frame-in-flight slot, the texture the slot's draw samples. Held until
    // the slot's fence retires so the pool cannot recycle it while in use.
    std::vector<std::shared_ptr<accelerator::vulkan::texture>> in_flight_textures_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_;

    std::atomic<bool> is_running_{true};
    std::atomic<bool> needs_resize_{false};
    std::atomic<bool> first_frame_presented_{false};
    std::thread       thread_;

    screen_consumer_vk(const screen_consumer_vk&)            = delete;
    screen_consumer_vk& operator=(const screen_consumer_vk&) = delete;

  public:
    screen_consumer_vk(const configuration&                                config,
                       const core::video_format_desc&                      format_desc,
                       int                                                 channel_index,
                       const std::shared_ptr<accelerator::vulkan::device>& device)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
        , device_(device)
    {
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

        frame_buffer_.set_capacity(1);

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

#if defined(_MSC_VER)
        DISPLAY_DEVICE              d_device = {sizeof(d_device), 0};
        std::vector<DISPLAY_DEVICE> displayDevices;
        for (int n = 0; EnumDisplayDevices(nullptr, n, &d_device, NULL); ++n) {
            displayDevices.push_back(d_device);
        }

        if (config_.screen_index >= displayDevices.size()) {
            CASPAR_LOG(warning) << print() << L" Invalid screen-index: " << config_.screen_index;
        }

        DEVMODE devmode = {};
        if (!EnumDisplaySettings(displayDevices[config_.screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &devmode)) {
            CASPAR_LOG(warning) << print() << L" Could not find display settings for screen-index: "
                                << config_.screen_index;
        }

        screen_x_      = devmode.dmPosition.x;
        screen_y_      = devmode.dmPosition.y;
        screen_width_  = devmode.dmPelsWidth;
        screen_height_ = devmode.dmPelsHeight;
#else
        if (config_.screen_index > 1) {
            CASPAR_LOG(warning) << print() << L" Screen-index is not supported on linux";
        }
#endif

        if (config.windowed) {
            screen_x_ = config.screen_x;
            screen_y_ = config.screen_y;

            if (config.screen_width > 0 && config.screen_height > 0) {
                screen_width_  = config.screen_width;
                screen_height_ = config.screen_height;
            } else if (config.screen_width > 0) {
                screen_width_  = config.screen_width;
                screen_height_ = square_height_ * config.screen_width / square_width_;
            } else if (config.screen_height > 0) {
                screen_height_ = config.screen_height;
                screen_width_  = square_width_ * config.screen_height / square_height_;
            } else {
                screen_width_  = square_width_;
                screen_height_ = square_height_;
            }
        }

        if (config_.sbs_key) {
            screen_width_ *= 2;
        }

        thread_ = std::thread([this] {
            try {
                run();
            } catch (tbb::user_abort&) {
                // Do nothing
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                is_running_ = false;
            }
        });
    }

    ~screen_consumer_vk()
    {
        is_running_ = false;
        frame_buffer_.abort();
        thread_.join();
    }

    void run()
    {
        window_config wcfg;
        wcfg.title         = u8(print());
        wcfg.x             = screen_x_;
        wcfg.y             = screen_y_;
        wcfg.width         = screen_width_;
        wcfg.height        = screen_height_;
        wcfg.windowed      = config_.windowed;
        wcfg.borderless    = config_.borderless;
        wcfg.interactive   = config_.interactive;
        wcfg.always_on_top = config_.always_on_top;
        wcfg.screen_index  = config_.screen_index;

        window_        = std::make_unique<screen_window>(wcfg);
        screen_width_  = window_->width();
        screen_height_ = window_->height();

        vk::Instance       instance = device_->instance();
        vk::PhysicalDevice physical = device_->physical_device();
        vk::Device         vkdevice = device_->getVkDevice();

        // The consumer presents on the same graphics queue the renderer/transfer
        // submit on (every hand-off is distance 0). There is no dedicated present
        // queue yet.
        queue_     = device_->queue();
        auto queue = queue_->vk_queue();

        // A command pool on the shared queue's family; render_pipeline allocates
        // its per-slot command buffers from it and resets them each frame.
        vk::CommandPoolCreateInfo pool_info{};
        pool_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_->family_index();
        command_pool_              = vkdevice.createCommandPool(pool_info);

        auto surface = window_->create_surface(instance);

        swapchain_ = std::make_unique<swapchain>(instance,
                                                 physical,
                                                 vkdevice,
                                                 queue,
                                                 queue_->family_index(),
                                                 surface,
                                                 config_.vsync,
                                                 [this](int& w, int& h) { window_->framebuffer_size(w, h); },
                                                 [this]() { window_->wait_for_events(); });

        render_pipeline_ = std::make_unique<render_pipeline>(vkdevice, physical, command_pool_, queue, *swapchain_);

        in_flight_textures_.resize(swapchain_->max_frames_in_flight());

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

        while (is_running_) {
            tick();
        }

        // Tear down GPU resources (render_pipeline waits for the device to be idle)
        // before destroying the command pool and window. The device is idle, so the
        // textures we were still holding are safe to release.
        render_pipeline_.reset();
        swapchain_.reset();
        in_flight_textures_.clear();
        // render_pipeline_.reset() above waited for the device to be idle, so the
        // pool has no in-flight buffers left.
        vkdevice.destroyCommandPool(command_pool_);
        queue_.reset();
        window_.reset();
    }

    void handle_resize()
    {
        if (!needs_resize_ || !is_running_)
            return;
        needs_resize_ = false;

        int width = 0, height = 0;
        window_->framebuffer_size(width, height);
        if (width == 0 || height == 0)
            return;

        screen_width_  = width;
        screen_height_ = height;

        auto lock = queue_->scoped_lock();
        swapchain_->recreate();
        render_pipeline_->recreate_framebuffers();
    }

    void tick()
    {
        core::const_frame in_frame;

        while (!frame_buffer_.try_pop(in_frame) && is_running_) {
            if (window_->poll())
                is_running_ = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        if (!in_frame || !is_running_)
            return;

        if (window_->poll())
            is_running_ = false;
        handle_resize();
        if (!is_running_)
            return;

        // GPU-direct: the channel's composited texture rides on the const_frame.
        // The vulkan image_mixer always provides one (the empty 1x1 texture for
        // empty frames) in eShaderReadOnlyOptimal, so a missing/foreign texture is
        // unexpected — skip it. On the single shared queue the mixer's finalize
        // barrier + submission order make the texture visible to our sample with no
        // token or semaphore (distance 0).
        auto src = std::dynamic_pointer_cast<accelerator::vulkan::texture>(in_frame.texture());
        if (!src) {
            CASPAR_LOG(warning) << print() << L" Frame has no Vulkan texture; skipping.";
            return;
        }

        auto params = calculate_render_params();

        {
            auto lock = queue_->scoped_lock();

            swapchain_->wait_for_fence();

            uint32_t image_index = swapchain_->acquire_next_image();
            if (image_index == std::numeric_limits<uint32_t>::max()) {
                needs_resize_ = true;
                return;
            }

            swapchain_->reset_fence();

            // The slot's fence (just waited) guarantees the previous draw in this
            // slot has retired, so its texture can be released and ours retained.
            uint32_t frame_slot             = swapchain_->frame_slot();
            in_flight_textures_[frame_slot] = src;

            render_pipeline_->render(*src,
                                     image_index,
                                     frame_slot,
                                     params,
                                     swapchain_->image_available_semaphore(),
                                     swapchain_->render_finished_semaphore(image_index),
                                     swapchain_->in_flight_fence());

            if (!swapchain_->present(image_index))
                needs_resize_ = true;

            swapchain_->next_frame();
        }

#ifdef __APPLE__
        if (!first_frame_presented_.exchange(true)) {
            window_->nudge_for_first_frame();
        }
#endif

        graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
        tick_timer_.restart();
    }

    screen_push_constants calculate_render_params()
    {
        screen_push_constants params{};

        float target_width  = 1.0f;
        float target_height = 1.0f;

        if (config_.stretch == screen::stretch::none) {
            target_width = static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) /
                           static_cast<float>(screen_width_);
            target_height = static_cast<float>(square_height_) / static_cast<float>(screen_height_);
        } else if (config_.stretch == screen::stretch::uniform) {
            float aspect = static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) /
                           static_cast<float>(square_height_);
            target_width =
                std::min(1.0f, static_cast<float>(screen_height_) * aspect / static_cast<float>(screen_width_));
            target_height =
                static_cast<float>(screen_width_ * target_width) / static_cast<float>(screen_height_ * aspect);
        } else if (config_.stretch == screen::stretch::uniform_to_fill) {
            float wr = static_cast<float>(config_.sbs_key ? square_width_ * 2 : square_width_) /
                       static_cast<float>(screen_width_);
            float hr      = static_cast<float>(square_height_) / static_cast<float>(screen_height_);
            float r_inv   = 1.0f / std::min(wr, hr);
            target_width  = wr * r_inv;
            target_height = hr * r_inv;
        }

        params.pos_scale[0]  = target_width;
        params.pos_scale[1]  = target_height;
        params.pos_offset[0] = 0.0f;
        params.pos_offset[1] = 0.0f;
        params.tex_scale[0]  = 1.0f;
        params.tex_scale[1]  = 1.0f;
        params.tex_offset[0] = 0.0f;
        params.tex_offset[1] = 0.0f;
        params.key_only      = config_.key_only ? 1 : 0;
        params.colour_space  = static_cast<int32_t>(config_.colour_space);
        params.window_width  = screen_width_;

        return params;
    }

    std::future<bool> send(core::video_field field, const core::const_frame& frame)
    {
        if (!frame_buffer_.try_push(frame)) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
        }
        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print() const { return config_.name + L" " + channel_and_format(); }
};

struct screen_consumer_proxy_vk : public core::frame_consumer
{
    const configuration                          config_;
    std::shared_ptr<accelerator::vulkan::device> device_;
    std::unique_ptr<screen_consumer_vk>          consumer_;

  public:
    screen_consumer_proxy_vk(configuration config, std::shared_ptr<accelerator::vulkan::device> device)
        : config_(std::move(config))
        , device_(std::move(device))
    {
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        consumer_.reset();
        consumer_ = std::make_unique<screen_consumer_vk>(config_, format_desc, channel_info.index, device_);
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        return consumer_->send(field, frame);
    }

    std::wstring print() const override { return consumer_ ? consumer_->print() : L"[vulkan screen consumer]"; }

    std::wstring name() const override { return L"screen"; }

    bool has_synchronization_clock() const override { return false; }

    // GPU-direct: we sample the composited texture off the const_frame, so the
    // channel can skip the GPU->host readback when we are the only consumer.
    bool needs_host_frame() const override { return false; }

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

spl::shared_ptr<core::frame_consumer> create_consumer(const std::shared_ptr<accelerator::vulkan::device>& device,
                                                      const std::vector<std::wstring>&                    params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !boost::iequals(params.at(0), L"SCREEN")) {
        return core::frame_consumer::empty();
    }

    auto config = parse_consumer_params(params, channel_info);

    return spl::make_shared<screen_consumer_proxy_vk>(config, device);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const std::shared_ptr<accelerator::vulkan::device>&      device,
                              const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    auto config = parse_preconfigured_consumer(ptree, channel_info);

    return spl::make_shared<screen_consumer_proxy_vk>(config, device);
}

}}} // namespace caspar::screen::vulkan
