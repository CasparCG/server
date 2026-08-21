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

#include <core/mixer/image/blend_modes.h>

#include <common/memory.h>
#include <functional>
#include <utility>
#include <vulkan/vulkan.hpp>

#include "../util/completion_token.h"
#include "../util/draw_params.h"
#include "../util/matrix.h"
#include "../util/transforms.h"
#include "../util/uniform_block.h"

namespace caspar { namespace accelerator { namespace vulkan {

class image_kernel final : public std::enable_shared_from_this<image_kernel>
{
    image_kernel(const image_kernel&);
    image_kernel& operator=(const image_kernel&);

  public:
    image_kernel(const spl::shared_ptr<class device>& device, common::bit_depth depth);
    ~image_kernel();

    spl::shared_ptr<class renderpass> create_renderpass(uint32_t width, uint32_t height);

    // Record + submit a one-time command buffer on the kernel's command context
    // (the same context/timeline the render uses), returning its completion. The
    // image_mixer drives the per-frame output finalize (the transition leaving the
    // composited target shader-read) through this, so the finalize shares one
    // timeline with the render.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record);

    // As above, but the submit first waits on the given completion tokens (cross-queue
    // hand-off consumer half). The image_mixer's readback finalize uses this to wait
    // the transfer queue's release before acquiring `target` back to shader-read.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record,
                                       vk::ArrayProxy<const completion_token>        wait_tokens);

    // A shared 1x1 transparent-black texture in shader-read layout, created at
    // kernel setup. Used as the GPU payload for empty frames (consumers sample a
    // valid cleared texture) and as the MoltenVK stand-in for absent planes.
    std::shared_ptr<class texture> empty_texture() const;

    // The completion of the most recent render submit ({timeline, last value}) on the kernel's
    // command context. A snapshot of the render queue's in-flight work, used to defer destruction of
    // a producer's command_context until any render batch still waiting on its tokens has drained.
    completion_token render_completion();

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
