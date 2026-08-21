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

#include "gpu_producer.h"

#include "barrier.h"         // transitionImageLayout
#include "command_context.h" // command_context
#include "handoff.h"         // handoff_token, record_release
#include "vulkan_queue.h"    // vulkan_queue (for *ctx_->queue())

#include <core/frame/frame_factory.h>

namespace caspar { namespace accelerator { namespace vulkan {

gpu_producer::gpu_producer(const spl::shared_ptr<core::frame_factory>& factory, queue_type queue)
{
    // The producer-side counterpart to the screen consumer's downcast: only the Vulkan
    // accelerator implements gpu_frame_factory. On any other accelerator gpu_ stays null and
    // operator bool reports false, so the producer can take its CPU-pixel fallback.
    gpu_ = dynamic_cast<gpu_frame_factory*>(factory.get());
    if (gpu_)
        ctx_ = gpu_->create_command_context(queue);
}

core::const_frame gpu_producer::produce(const void*                    tag,
                                        std::vector<producer_plane>    planes,
                                        const core::pixel_format_desc& desc,
                                        const record_fn&               record,
                                        array<const std::int32_t>      audio)
{
    // One hand-off per plane (planes may differ in scope, e.g. NV12 Y vs UV); they all share
    // the single completion token of the one submit below. completion is empty here — filled
    // after the submit returns.
    std::vector<handoff_token> handoffs;
    handoffs.reserve(planes.size());
    for (const auto& p : planes) {
        handoffs.push_back(gpu_->make_producer_handoff(*ctx_->queue(), p.work_layout, p.work_stage, p.work_access));
    }

    // The textures the workload fills, in plane order.
    std::vector<std::shared_ptr<texture>> textures;
    textures.reserve(planes.size());
    for (const auto& p : planes)
        textures.push_back(p.tex);

    auto token = ctx_->record_and_submit([&](vk::CommandBuffer cmd) {
        // Acquire: move each texture from its incoming layout into the work scope the
        // producer declared. Source scope (eTopOfPipe, eNone) suits a fresh/discardable
        // texture; the dst scope is the same triple the release uses, so they cannot drift.
        for (const auto& p : planes) {
            transitionImageLayout(p.tex->id(),
                                  p.from_layout,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  p.work_layout,
                                  p.work_access,
                                  p.work_stage,
                                  cmd);
        }

        record(cmd, textures);

        // Release: the producer->render boundary for each texture (a plain transition at
        // distance 0/1, a queue-family release at distance 2).
        for (std::size_t i = 0; i < planes.size(); ++i)
            record_release(cmd, handoffs[i], planes[i].tex->id());
    });

    // Stamp the shared completion onto every hand-off so the renderer waits it
    // (inert at distance 0).
    for (auto& h : handoffs)
        h.completion = token;

    std::vector<gpu_plane> gpu_planes;
    gpu_planes.reserve(planes.size());
    for (std::size_t i = 0; i < planes.size(); ++i)
        gpu_planes.push_back(gpu_plane{std::move(planes[i].tex), handoffs[i]});

    return gpu_->import_textures(tag, std::move(gpu_planes), desc, std::move(audio));
}

core::const_frame
gpu_producer::produce(const void*                                                                    tag,
                      producer_plane                                                                 plane,
                      core::pixel_format                                                             fmt,
                      const std::function<void(vk::CommandBuffer, const std::shared_ptr<texture>&)>& record,
                      array<const std::int32_t>                                                      audio)
{
    core::pixel_format_desc desc(fmt);
    desc.planes.push_back(core::pixel_format_desc::plane(plane.tex->width(), plane.tex->height(), plane.tex->stride()));

    std::vector<producer_plane> planes;
    planes.push_back(std::move(plane));

    return produce(
        tag,
        std::move(planes),
        desc,
        [&](vk::CommandBuffer cmd, const std::vector<std::shared_ptr<texture>>& textures) {
            record(cmd, textures.front());
        },
        std::move(audio));
}

}}} // namespace caspar::accelerator::vulkan
