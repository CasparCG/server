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

#pragma once

#include "queue_manager.h" // queue_type
#include "texture.h"       // texture, texture_usage, handoff_token (via handoff.h)

#include <common/array.h>
#include <common/bit_depth.h>

#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class command_context;
class vulkan_queue;

// One image plane handed over by a GPU producer: a texture the producer already created and recorded
// on its own queue, plus the hand-off describing how it moves to the render queue (the producer fills
// `handoff.completion` after submitting). A bare shared_ptr<texture> — no future, the texture exists.
struct gpu_plane
{
    std::shared_ptr<texture> tex;
    handoff_token            handoff;
};

// The producer-side counterpart to the const_frame::texture() consumer path: lets a GPU-aware
// producer hand the mixer texture(s) that are already on the GPU (compute, hardware decode, ...)
// instead of CPU pixels the transfer service uploads. A producer obtains it the same way the screen
// consumer obtains vulkan::texture — by downcasting the core::frame_factory it already holds:
//
//     auto* gpu = dynamic_cast<vulkan::gpu_frame_factory*>(frame_factory_.get());
//     if (!gpu) { /* non-Vulkan accelerator: fall back to create_frame + CPU pixels */ }
//
// The factory hands out the building blocks; the producer OWNS its recording. Typical use: in its
// constructor a producer mints a command_context for its queue; each frame it creates a texture,
// asks the mixer for the hand-off with make_producer_handoff(*ctx->queue(), ...), records its work +
// the producer->render release on its own context, and wraps the result with import_textures. The
// producer never names the render queue or its consumer-side scope — the mixer owns those. The
// mixer keeps no per-producer state — the textures ride
// on const_frame::opaque() like uploaded ones, so they flow through visit()/renderpass unchanged and
// the existing handoff machinery acquires them on the render queue.
class gpu_frame_factory
{
  public:
    virtual ~gpu_frame_factory() = default;

    // Allocate a GPU texture for the producer to write and the renderer to sample. EXCLUSIVE, initial
    // layout eUndefined (start the producer's first barrier from eUndefined / discard). Pass
    // texture_usage::storage for compute imageStore writes.
    virtual std::shared_ptr<texture>
    create_producer_texture(int width, int height, int stride, common::bit_depth depth) = 0;

    // Mint a fresh command_context on the dedicated queue for `queue` (the render queue's alias when
    // the hardware has no dedicated family; null for an unsupported video queue). The caller OWNS it —
    // hold it for the producer's lifetime. Get its queue back via command_context::queue() to build
    // the producer->render hand-off with make_producer_handoff(*ctx->queue(), ...).
    virtual std::shared_ptr<command_context> create_command_context(queue_type queue) = 0;

    // Build the producer->render hand-off for a texture the producer is about to release. The
    // producer supplies only what it knows — its own queue and the layout/scope it leaves the texture
    // in; the mixer supplies the render queue and its sampling scope (shader-read in the fragment
    // stage). The producer fills `completion` after submitting, then records record_release() in that
    // submit.
    virtual handoff_token make_producer_handoff(const vulkan_queue&     producer_queue,
                                                vk::ImageLayout         src_layout,
                                                vk::PipelineStageFlags2 src_stage,
                                                vk::AccessFlags2        src_access) = 0;

    // Wrap already-produced, already-recorded textures into a const_frame. Stamps each plane's
    // hand-off onto its texture so the renderer acquires it. `desc` must be valid and describe one
    // plane per texture (e.g. a single bgra plane, or two planes for NV12).
    virtual core::const_frame import_textures(const void*                    tag,
                                              std::vector<gpu_plane>         planes,
                                              const core::pixel_format_desc& desc,
                                              array<const std::int32_t>      audio = {}) = 0;
};

}}} // namespace caspar::accelerator::vulkan
