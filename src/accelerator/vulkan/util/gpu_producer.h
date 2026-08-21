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

#include "gpu_frame_factory.h" // gpu_frame_factory, gpu_plane, queue_type
#include "texture.h"           // texture, handoff_token (via handoff.h)

#include <common/array.h>
#include <common/bit_depth.h>
#include <common/memory.h>

#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace core {
class frame_factory;
}} // namespace caspar::core

namespace caspar { namespace accelerator { namespace vulkan {

class command_context;

// One already-created texture a GPU producer is handing over, plus the scope it LEAVES it
// in. One per import plane. The producer owns `tex`'s allocation — gpu_producer never
// creates textures (call gpu_producer::factory().create_producer_texture(...), reuse a pool,
// import a hardware-decode image, ...).
//
// `work_*` is fed to BOTH halves of the producer->render hand-off: it is the destination
// scope of the acquire transition gpu_producer records before the workload, and the source
// scope of make_producer_handoff()/record_release() after it. Because there is a single
// declaration, the acquire and the release can never drift. Declare `work_*` as the
// producer's LAST use of the texture (what the mixer must synchronize against).
//
// Caveat (first-use vs last-use): the acquire from `from_layout` (eUndefined discards
// contents) into the last-use scope is a safe over-approximation. A producer that touches
// the texture in more than one stage (e.g. a transfer clear then a compute write) records
// that one intermediate transitionImageLayout itself inside its workload lambda; the
// cross-queue boundary is still described by exactly one scope.
struct producer_plane
{
    std::shared_ptr<texture> tex;

    // Current layout of `tex` entering this frame; the acquire transitions from it into the
    // work scope with a source scope of (eTopOfPipe, eNone) — correct for a fresh /
    // discardable texture, which is what create_producer_texture() returns.
    vk::ImageLayout from_layout = vk::ImageLayout::eUndefined;

    // The layout/scope the producer's workload leaves the texture in. Defaults match the
    // common case (a transfer clear/copy into eGeneral).
    vk::ImageLayout         work_layout = vk::ImageLayout::eGeneral;
    vk::PipelineStageFlags2 work_stage  = vk::PipelineStageFlagBits2::eTransfer;
    vk::AccessFlags2        work_access = vk::AccessFlagBits2::eTransferWrite;
};

// The per-producer counterpart to the raw gpu_frame_factory API: a small holder that owns
// the gpu_frame_factory downcast + the producer's command_context, and folds the entire
// per-frame hand-off sequence into produce(). It removes every load-bearing step a producer
// would otherwise repeat (and could silently get wrong): the acquire transition, the
// per-texture record_release, stamping completion onto every hand-off after submit, and the
// gpu_plane/import_textures assembly. Texture creation is intentionally NOT hidden.
//
// Construct once from the producer's frame_factory and check operator bool — false means the
// channel is not on the Vulkan accelerator and the producer should take its CPU-pixel
// fallback. gpu_producer does NOT throw on the non-Vulkan case; that policy belongs to the
// producer (some legitimately fall back, the gpu_test producer throws).
//
//     gpu_producer gpu_{frame_factory_, queue_type::compute};
//     if (!gpu_) { /* CPU fallback */ }
//     ...
//     auto tex = gpu_.factory().create_producer_texture(w, h, 4, bit_depth::bit8);
//     return core::draw_frame(gpu_.produce(this, producer_plane{std::move(tex)},
//                                          core::pixel_format::rgba,
//                                          [&](vk::CommandBuffer cmd, const auto& tex){ /* fill */ }));
class gpu_producer
{
  public:
    gpu_producer() = default; // empty; operator bool == false
    gpu_producer(const spl::shared_ptr<core::frame_factory>& factory, queue_type queue);

    explicit operator bool() const noexcept { return gpu_ != nullptr; }

    // Records ONLY content generation. The acquire transition for every texture is already
    // recorded before this runs (each texture is in its work_layout, ready to write); the
    // release for every texture is recorded after. `textures` are the planes' tex in order.
    using record_fn = std::function<void(vk::CommandBuffer, const std::vector<std::shared_ptr<texture>>&)>;

    // N-plane primitive (NV12 etc.). `desc` must describe one plane per texture, in order.
    // Builds a per-plane hand-off, records acquire + `record` + release on one submit, stamps
    // the shared completion onto every hand-off, and wraps the result with import_textures.
    core::const_frame produce(const void*                    tag,
                              std::vector<producer_plane>    planes,
                              const core::pixel_format_desc& desc,
                              const record_fn&               record,
                              array<const std::int32_t>      audio = {});

    // 1-plane convenience: derives the single-plane pixel_format_desc from the texture's
    // geometry and `fmt`, and passes the lone texture straight to `record` so the common
    // producer never indexes a vector. Forwards to the N-plane primitive.
    core::const_frame produce(const void*                                                                    tag,
                              producer_plane                                                                 plane,
                              core::pixel_format                                                             fmt,
                              const std::function<void(vk::CommandBuffer, const std::shared_ptr<texture>&)>& record,
                              array<const std::int32_t> audio = {});

    gpu_frame_factory& factory() const { return *gpu_; } // for create_producer_texture(...)
    command_context&   context() const { return *ctx_; }

  private:
    gpu_frame_factory*               gpu_ = nullptr;
    std::shared_ptr<command_context> ctx_;
};

}}} // namespace caspar::accelerator::vulkan
