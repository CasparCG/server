/*
 * Copyright 2025
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
#include "image_mixer.h"

#include "image_kernel.h"

#include "../util/barrier.h"
#include "../util/buffer.h"
#include "../util/command_context.h"
#include "../util/device.h"
#include "../util/gpu_frame_factory.h"
#include "../util/handoff.h"
#include "../util/renderpass.h"
#include "../util/texture.h"
#include "../util/transfer.h"
#include "../util/vulkan_queue.h"

#ifdef WIN32
#include "../../d3d/d3d_texture2d.h"
#endif

#include <boost/align/aligned_allocator.hpp>

#include <common/array.h>
#include <common/bit_depth.h>
#include <common/future.h>
#include <common/log.h>

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <algorithm>
#include <any>
#include <memory>
#include <mutex>
#include <vector>

namespace caspar { namespace accelerator { namespace vulkan {

// Textures are created eagerly (transfer::copy_async returns the texture directly, GPU ordering
// rides the handoff/timeline token), so we carry the resolved texture — no future wrapper, no CPU
// wait. This is Vulkan-internal; the cross-accelerator boundary is the type-erased const_frame
// opaque() (std::any). The OGL accelerator keeps its own genuinely-async future_texture.
using texture_ptr = std::shared_ptr<texture>;

struct item
{
    core::pixel_format_desc  pix_desc = core::pixel_format_desc(core::pixel_format::invalid);
    std::vector<texture_ptr> textures;
    draw_transforms          transforms;
    core::frame_geometry     geometry = core::frame_geometry::get_default();
};

struct layer
{
    std::vector<layer> sublayers;
    std::vector<item>  items;
    core::blend_mode   blend_mode;

    explicit layer(core::blend_mode blend_mode)
        : blend_mode(blend_mode)
    {
    }
};

class image_renderer
{
    spl::shared_ptr<device> vulkan_;
    image_kernel            kernel_;
    const size_t            max_frame_size_;
    common::bit_depth       depth_;

  public:
    explicit image_renderer(const spl::shared_ptr<device>& vulkan, const size_t max_frame_size, common::bit_depth depth)
        : vulkan_(vulkan)
        , kernel_(vulkan_, depth)
        , max_frame_size_(max_frame_size)
        , depth_(depth)
    {
    }

    std::future<std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>>>
    operator()(std::vector<layer> layers, const core::video_format_desc& format_desc, bool need_host_frame)
    {
        if (layers.empty()) { // Bypass GPU with empty frame.
            static const std::vector<uint8_t, boost::alignment::aligned_allocator<uint8_t, 32>> buffer(max_frame_size_,
                                                                                                       0);
            return make_ready_future<std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>>>(
                {array<const std::uint8_t>(buffer.data(), format_desc.size, true), kernel_.empty_texture()});
        }

        // Record + submit synchronously on the caller's (mixer) thread; the only
        // CPU wait is the readback future's .get() downstream (it consumes bytes).
        auto pass   = kernel_.create_renderpass(format_desc.square_width, format_desc.square_height);
        auto target = pass->default_attachment();
        draw(target, std::move(layers), format_desc, pass);

        pass->commit(); // leaves `target` in eRenderingLocalRead

        if (!need_host_frame) {
            // Finalize to the output invariant: `target` always ends in
            // eShaderReadOnlyOptimal
            kernel_.record_and_submit([&](vk::CommandBuffer cmd) {
                transitionImageLayout(target->id(),
                                      vk::ImageLayout::eRenderingLocalRead,
                                      vk::AccessFlagBits2::eColorAttachmentWrite,
                                      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                      vk::ImageLayout::eShaderReadOnlyOptimal,
                                      vk::AccessFlagBits2::eShaderRead,
                                      vk::PipelineStageFlagBits2::eAllCommands,
                                      cmd);
            });

            return make_ready_future<std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>>>(
                {array<const std::uint8_t>(), target});
        }

        // A host consumer wants the bytes: read them back through the transfer
        // service on its own command context and (at distance 1/2) its own queue.
        // The readback is a two-leg cross-queue hand-off: the render queue RELEASES
        // `target` (eRenderingLocalRead -> eTransferSrcOptimal) to the transfer queue,
        // which copies it out and hands it back (eTransferSrcOptimal ->
        // eShaderReadOnlyOptimal), so a GPU-direct consumer can still sample the
        // const_frame's texture. At distance 0 every leg is inert and this is
        // byte-for-byte the old submission-ordered sequence.
        auto to_transfer = vulkan_->transfer().readback_handoff();
        auto release_token =
            kernel_.record_and_submit([&](vk::CommandBuffer cmd) { record_release(cmd, to_transfer, target->id()); });
        to_transfer.completion = release_token;

        auto readback = vulkan_->transfer().copy_async(target, to_transfer);

        // Finalize: take the transfer service's return hand-off and record its acquire
        // half on the kernel context, waiting the readback's completion, so `target`
        // ends in the shader-read output invariant the screen consumer samples.
        auto to_renderer = target->take_pending_handoff();
        kernel_.record_and_submit([&](vk::CommandBuffer cmd) { acquire_into(cmd, to_renderer, target->id()); },
                                  to_renderer.completion);

        return std::async(std::launch::deferred,
                          [readback = std::move(readback),
                           target]() mutable -> std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>> {
                              return {std::move(readback.get()), target};
                          });
    }

    common::bit_depth depth() const { return depth_; }

    completion_token render_completion() { return kernel_.render_completion(); }

  private:
    void draw(std::shared_ptr<texture>&      target_texture,
              std::vector<layer>             layers,
              const core::video_format_desc& format_desc,
              spl::shared_ptr<renderpass>    pass)
    {
        std::shared_ptr<texture> layer_key_texture;

        for (auto& layer : layers) {
            draw(target_texture, layer.sublayers, format_desc, pass);
            draw(target_texture, std::move(layer), layer_key_texture, format_desc, pass);
        }
    }

    void draw(std::shared_ptr<texture>&      target_texture,
              layer                          layer,
              std::shared_ptr<texture>&      layer_key_texture,
              const core::video_format_desc& format_desc,
              spl::shared_ptr<renderpass>    pass)
    {
        if (layer.items.empty())
            return;

        std::shared_ptr<texture> local_key_texture;
        std::shared_ptr<texture> local_mix_texture;

        if (layer.blend_mode != core::blend_mode::normal) {
            auto layer_texture = pass->create_attachment();

            for (auto& item : layer.items)
                draw(layer_texture,
                     std::move(item),
                     layer_key_texture,
                     local_key_texture,
                     local_mix_texture,
                     format_desc,
                     pass);

            draw(layer_texture, std::move(local_mix_texture), format_desc, pass, core::blend_mode::normal);
            draw(target_texture, std::move(layer_texture), format_desc, pass, layer.blend_mode);
        } else // fast path
        {
            for (auto& item : layer.items)
                draw(target_texture,
                     std::move(item),
                     layer_key_texture,
                     local_key_texture,
                     local_mix_texture,
                     format_desc,
                     pass);

            draw(target_texture, std::move(local_mix_texture), format_desc, pass, core::blend_mode::normal);
        }

        layer_key_texture = std::move(local_key_texture);
    }

    void draw(std::shared_ptr<texture>&      target_texture,
              item                           item,
              std::shared_ptr<texture>&      layer_key_texture,
              std::shared_ptr<texture>&      local_key_texture,
              std::shared_ptr<texture>&      local_mix_texture,
              const core::video_format_desc& format_desc,
              spl::shared_ptr<renderpass>    pass)
    {
        draw_params draw_params;
        draw_params.target_width  = format_desc.square_width;
        draw_params.target_height = format_desc.square_height;
        // TODO: Pass the target color_space

        draw_params.pix_desc   = std::move(item.pix_desc);
        draw_params.transforms = std::move(item.transforms);
        draw_params.geometry   = std::move(item.geometry);
        draw_params.aspect_ratio =
            static_cast<double>(format_desc.square_width) / static_cast<double>(format_desc.square_height);

        for (auto& tex : item.textures) {
            draw_params.textures.push_back(spl::make_shared_ptr(tex));
        }

        if (draw_params.transforms.image_transform
                .is_key) { // A key means we will use it for the next non-key item as a mask
            local_key_texture = local_key_texture ? local_key_texture : pass->create_attachment();

            draw_params.background = local_key_texture;
            draw_params.local_key  = nullptr;
            draw_params.layer_key  = nullptr;

            pass->draw(std::move(draw_params));
        } else if (draw_params.transforms.image_transform
                       .is_mix) { // A mix means precomp the items to a texture, before drawing to the channel
            local_mix_texture = local_mix_texture ? local_mix_texture : pass->create_attachment();

            draw_params.background = local_mix_texture;
            draw_params.local_key  = std::move(local_key_texture); // Use and reset the key
            draw_params.layer_key  = layer_key_texture;

            draw_params.keyer = keyer::additive;

            pass->draw(std::move(draw_params));
        } else {
            // If there is a mix, this is the end so draw it and reset
            draw(target_texture, std::move(local_mix_texture), format_desc, pass, core::blend_mode::normal);

            draw_params.background = target_texture;
            draw_params.local_key  = std::move(local_key_texture);
            draw_params.layer_key  = layer_key_texture;

            pass->draw(std::move(draw_params));
        }
    }

    void draw(std::shared_ptr<texture>&   target_texture,
              std::shared_ptr<texture>&&  source_texture,
              core::video_format_desc     format_desc,
              spl::shared_ptr<renderpass> pass,
              core::blend_mode            blend_mode = core::blend_mode::normal)
    {
        if (!source_texture)
            return;

        draw_params draw_params;
        draw_params.target_width    = format_desc.square_width;
        draw_params.target_height   = format_desc.square_height;
        draw_params.pix_desc.format = core::pixel_format::bgra;
        draw_params.pix_desc.planes = {core::pixel_format_desc::plane(
            source_texture->width(), source_texture->height(), 4, source_texture->depth())};
        draw_params.textures        = {spl::make_shared_ptr(source_texture)};
        draw_params.blend_mode      = blend_mode;
        draw_params.background      = target_texture;
        draw_params.geometry        = core::frame_geometry::get_default();

        pass->draw(std::move(draw_params));
    }
};

struct image_mixer::impl
    : public core::frame_factory
    , public std::enable_shared_from_this<impl>
{
    // A producer-owned command_context awaiting deferred destruction: kept alive until the GPU has
    // drained both its own work (`self`) and the render queue's work in flight when it was handed back
    // (`render_barrier`) — which may still wait on the completion_tokens its timeline signalled.
    struct retired_context
    {
        std::unique_ptr<command_context> ctx;
        completion_token                 self;
        completion_token                 render_barrier;
    };

    spl::shared_ptr<device>      vulkan_;
    image_renderer               renderer_;
    std::vector<draw_transforms> transform_stack_;
    std::vector<layer>           layers_; // layer/stream/items
    std::vector<layer*>          layer_stack_;
    std::mutex                   retire_mutex_;
    std::vector<retired_context> retired_;

    double aspect_ratio_ = 1.0;

  public:
    impl(const spl::shared_ptr<device>& device,
         const int                      channel_id,
         const size_t                   max_frame_size,
         common::bit_depth              depth)
        : vulkan_(device)
        , renderer_(device, max_frame_size, depth)
        , transform_stack_(1)
    {
        CASPAR_LOG(info) << L"Initialized Vulkan Accelerated GPU Image Mixer for channel " << channel_id;
    }

    ~impl()
    {
        // Destroy any producer contexts still awaiting drain. The render thread has stopped by now, so
        // idle the device — command_context's dtor does not wait, and a retired one may not have been
        // polled to completion yet.
        vulkan_->getVkDevice().waitIdle();
        retired_.clear();
    }

    void update_aspect_ratio(double aspect_ratio) { aspect_ratio_ = aspect_ratio; }

    void push(const core::frame_transform& transform)
    {
        auto previous_layer_depth = transform_stack_.back().image_transform.layer_depth;

        transform_stack_.push_back(transform_stack_.back().combine_transform(transform.image_transform, aspect_ratio_));

        auto new_layer_depth = transform_stack_.back().image_transform.layer_depth;

        if (previous_layer_depth < new_layer_depth) {
            layer new_layer(transform_stack_.back().image_transform.blend_mode);

            if (layer_stack_.empty()) {
                layers_.push_back(std::move(new_layer));
                layer_stack_.push_back(&layers_.back());
            } else {
                layer_stack_.back()->sublayers.push_back(std::move(new_layer));
                layer_stack_.push_back(&layer_stack_.back()->sublayers.back());
            }
        }
    }

    void visit(const core::const_frame& frame)
    {
        if (frame.pixel_format_desc().format == core::pixel_format::invalid)
            return;

        if (frame.pixel_format_desc().planes.empty())
            return;

        item item;
        item.pix_desc   = frame.pixel_format_desc();
        item.transforms = transform_stack_.back();
        item.geometry   = frame.geometry();

        // A GPU-resident frame carries its textures out-of-band in opaque(); a host frame does not.
        // This any_cast is what tells the two apart. Use the pointer overload (note the &): on a
        // host frame it just returns nullptr, so we fall through to the host path below.
        const auto* gpu_textures = std::any_cast<std::shared_ptr<std::vector<texture_ptr>>>(&frame.opaque());

        if (gpu_textures && *gpu_textures) {
            item.textures = **gpu_textures;
        } else {
            for (int n = 0; n < static_cast<int>(item.pix_desc.planes.size()); ++n) {
                item.textures.emplace_back(vulkan_->transfer().copy_async(frame.image_data(n),
                                                                          item.pix_desc.planes[n].width,
                                                                          item.pix_desc.planes[n].height,
                                                                          item.pix_desc.planes[n].stride,
                                                                          item.pix_desc.planes[n].depth));
            }
        }

        layer_stack_.back()->items.push_back(item);
    }

    void pop()
    {
        transform_stack_.pop_back();
        layer_stack_.resize(transform_stack_.back().image_transform.layer_depth);
    }

    std::future<std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>>>
    render(const core::video_format_desc& format_desc, bool need_host_frame)
    {
        drain_retired(); // reclaim any producer contexts the GPU has finished with
        return renderer_(std::move(layers_), format_desc, need_host_frame);
    }

    core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override
    {
        return create_frame(tag, desc, common::bit_depth::bit8);
    }

    core::mutable_frame
    create_frame(const void* tag, const core::pixel_format_desc& desc, common::bit_depth depth) override
    {
        std::vector<array<std::uint8_t>> image_data;
        for (auto& plane : desc.planes) {
            auto bytes_per_pixel = depth == common::bit_depth::bit8 ? 1 : 2;
            image_data.push_back(vulkan_->create_array(plane.size * bytes_per_pixel));
        }

        std::weak_ptr<image_mixer::impl> weak_self = shared_from_this();
        return core::mutable_frame(tag,
                                   std::move(image_data),
                                   array<int32_t>{},
                                   desc,
                                   [weak_self, desc](std::vector<array<const std::uint8_t>> image_data) -> std::any {
                                       auto self = weak_self.lock();
                                       if (!self) {
                                           return std::any{};
                                       }
                                       std::vector<texture_ptr> textures;
                                       for (int n = 0; n < static_cast<int>(desc.planes.size()); ++n) {
                                           textures.emplace_back(
                                               self->vulkan_->transfer().copy_async(image_data[n],
                                                                                    desc.planes[n].width,
                                                                                    desc.planes[n].height,
                                                                                    desc.planes[n].stride,
                                                                                    desc.planes[n].depth));
                                       }
                                       return std::make_shared<decltype(textures)>(std::move(textures));
                                   });
    }

#ifdef WIN32
    core::const_frame import_d3d_texture(const void*                                tag,
                                         const std::shared_ptr<d3d::d3d_texture2d>& d3d_texture,
                                         core::pixel_format                         format,
                                         common::bit_depth                          depth) override
    {
        throw std::runtime_error("d3d texture import not supported on vulkan accelerator");
    }
#endif

    // --- gpu_frame_factory (GPU producer path) ---

    std::shared_ptr<texture> create_producer_texture(int width, int height, int stride, common::bit_depth depth)
    {
        return vulkan_->create_texture(width, height, stride, depth);
    }

    std::shared_ptr<command_context> create_command_context(queue_type queue)
    {
        auto q = vulkan_->acquire_queue(queue);
        if (!q)
            return nullptr;
        // The producer holds this for its lifetime but must NOT destroy it inline: when it dies the
        // render queue may still be waiting on completion_tokens this context's timeline signalled, so
        // tearing down the timeline semaphore would be use-after-free. The custom deleter hands it back
        // for deferred destruction once the GPU has drained that work (see retire_command_context).
        auto* raw = new command_context(vulkan_->getVkDevice(), q);
        return std::shared_ptr<command_context>(raw, [this](command_context* ctx) { retire_command_context(ctx); });
    }

    // Take ownership of a producer's expired command_context (called from its shared_ptr deleter) and
    // queue it for destruction once the GPU is done with it. Snapshots the drain conditions now;
    // drain_retired() polls them on the render thread.
    void retire_command_context(command_context* ctx)
    {
        completion_token self           = ctx->current_completion();    // this context's own submits
        completion_token render_barrier = renderer_.render_completion(); // render work that may await it

        std::lock_guard<std::mutex> lock(retire_mutex_);
        retired_.push_back({std::unique_ptr<command_context>(ctx), self, render_barrier});
    }

    // Destroy every retired context whose drain conditions the GPU has now passed. Non-blocking
    // (vkGetSemaphoreCounterValue only); called once per render tick from render().
    void drain_retired()
    {
        auto device  = vulkan_->getVkDevice();
        auto reached = [&](const completion_token& t) {
            return !t.timeline || device.getSemaphoreCounterValue(t.timeline) >= t.value;
        };

        std::lock_guard<std::mutex> lock(retire_mutex_);
        retired_.erase(std::remove_if(retired_.begin(),
                                      retired_.end(),
                                      [&](retired_context& r) { return reached(r.self) && reached(r.render_barrier); }),
                       retired_.end());
    }

    handoff_token make_producer_handoff(const vulkan_queue&     producer_queue,
                                        vk::ImageLayout         src_layout,
                                        vk::PipelineStageFlags2 src_stage,
                                        vk::AccessFlags2        src_access)
    {
        return make_handoff(producer_queue,
                            *vulkan_->queue(),
                            src_layout,
                            vk::ImageLayout::eShaderReadOnlyOptimal,
                            src_stage,
                            src_access,
                            vk::PipelineStageFlagBits2::eFragmentShader,
                            vk::AccessFlagBits2::eShaderRead);
    }

    core::const_frame import_textures(const void*                    tag,
                                      std::vector<gpu_plane>         planes,
                                      const core::pixel_format_desc& desc,
                                      array<const std::int32_t>      audio)
    {
        auto textures = std::make_shared<std::vector<texture_ptr>>();
        textures->reserve(planes.size());
        for (auto& p : planes) {
            // Stamp the producer->render hand-off so renderpass acquires it (inert at distance 0).
            p.tex->set_pending_handoff(p.handoff);
            textures->push_back(std::move(p.tex));
        }
        return core::const_frame::from_textures(tag, desc, std::any(std::move(textures)), std::move(audio));
    }

    common::bit_depth depth() const { return renderer_.depth(); }
};

image_mixer::image_mixer(const spl::shared_ptr<device>& vulkan,
                         const int                      channel_id,
                         const size_t                   max_frame_size,
                         common::bit_depth              depth)
    : impl_(std::make_unique<impl>(vulkan, channel_id, max_frame_size, depth))
{
}
image_mixer::~image_mixer() {}
void image_mixer::push(const core::frame_transform& transform) { impl_->push(transform); }
void image_mixer::visit(const core::const_frame& frame) { impl_->visit(frame); }
void image_mixer::pop() { impl_->pop(); }
void image_mixer::update_aspect_ratio(double aspect_ratio) { impl_->update_aspect_ratio(aspect_ratio); }
std::future<std::tuple<array<const std::uint8_t>, std::shared_ptr<core::texture>>>
image_mixer::render(const core::video_format_desc& format_desc, bool need_host_frame)
{
    return impl_->render(format_desc, need_host_frame);
}
core::mutable_frame image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc)
{
    return impl_->create_frame(tag, desc);
}
core::mutable_frame
image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc, common::bit_depth depth)
{
    return impl_->create_frame(tag, desc, depth);
}

#ifdef WIN32
core::const_frame image_mixer::import_d3d_texture(const void*                                tag,
                                                  const std::shared_ptr<d3d::d3d_texture2d>& d3d_texture,
                                                  core::pixel_format                         format,
                                                  common::bit_depth                          depth)
{
    return impl_->import_d3d_texture(tag, d3d_texture, format, depth);
}
#endif

std::shared_ptr<texture>
image_mixer::create_producer_texture(int width, int height, int stride, common::bit_depth depth)
{
    return impl_->create_producer_texture(width, height, stride, depth);
}
std::shared_ptr<command_context> image_mixer::create_command_context(queue_type queue)
{
    return impl_->create_command_context(queue);
}
handoff_token image_mixer::make_producer_handoff(const vulkan_queue&     producer_queue,
                                                 vk::ImageLayout         src_layout,
                                                 vk::PipelineStageFlags2 src_stage,
                                                 vk::AccessFlags2        src_access)
{
    return impl_->make_producer_handoff(producer_queue, src_layout, src_stage, src_access);
}
core::const_frame             image_mixer::import_textures(const void*                    tag,
                                               std::vector<gpu_plane>         planes,
                                               const core::pixel_format_desc& desc,
                                               array<const std::int32_t>      audio)
{
    return impl_->import_textures(tag, std::move(planes), desc, std::move(audio));
}

common::bit_depth image_mixer::depth() const { return impl_->depth(); }

}}} // namespace caspar::accelerator::vulkan
