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

#include "handoff.h"

#include <common/bit_depth.h>
#include <core/frame/frame.h>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class texture final : public core::texture
{
  public:
    texture(int               width,
            int               height,
            int               stride,
            common::bit_depth depth,
            vk::Image         image,
            vk::DeviceMemory  memory,
            vk::ImageView     imageView,
            vk::Device        device);
    texture(const texture&) = delete;
    texture(texture&& other);
    ~texture();

    texture& operator=(const texture&) = delete;
    texture& operator=(texture&& other);

    vk::ImageView view() const;

    // core::texture interface. No-ops on Vulkan: there is no global texture bind
    // point — sampling is expressed via descriptor sets at draw time.
    void bind(int index) override;
    void unbind() override;

    int               width() const;
    int               height() const;
    int               stride() const;
    common::bit_depth depth() const;
    void              set_depth(common::bit_depth depth);
    int               size() const;
    VkImage           id() const;

    // The pending cross-queue hand-off for this texture: stamped by the producer
    // (the transfer upload's release half) and consumed once by the next-stage
    // consumer (the renderer's acquire half), which both waits its completion and
    // records the matching acquire barrier. The only per-frame multi-queue state on
    // the texture — empty/inert when producer and consumer share a queue.
    void          set_pending_handoff(const handoff_token& token);
    handoff_token take_pending_handoff(); // moves it out, leaving an empty token

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
