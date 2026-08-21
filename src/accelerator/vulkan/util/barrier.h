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

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

// Record a single-subresource image layout transition (color, mip 0, layer 0)
// with explicit src/dst access + stage masks AND explicit queue families. With
// real (non-Ignored) families this is one half of a queue-family ownership
// transfer (QFOT): the producer records a RELEASE (its last-use src scope, empty
// dst scope) and the consumer records the matching ACQUIRE (empty src scope, its
// first-use dst scope) — identical oldLayout/newLayout, families and range on both
// halves so they pair. See handoff.{h,cpp}, which is the only QFOT caller.
inline void transitionImageLayoutQFOT(const vk::Image&        image,
                                      vk::ImageLayout         oldLayout,
                                      vk::AccessFlags2        srcAccessMask,
                                      vk::PipelineStageFlags2 srcStage,
                                      vk::ImageLayout         newLayout,
                                      vk::AccessFlags2        dstAccessMask,
                                      vk::PipelineStageFlags2 dstStage,
                                      uint32_t                srcQueueFamily,
                                      uint32_t                dstQueueFamily,
                                      vk::CommandBuffer       cmdBuffer)
{
    auto range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    vk::ImageMemoryBarrier2 barrier{};
    barrier.oldLayout = oldLayout, barrier.newLayout = newLayout, barrier.srcQueueFamilyIndex = srcQueueFamily,
    barrier.dstQueueFamilyIndex = dstQueueFamily, barrier.image = image, barrier.subresourceRange = range;

    barrier.srcAccessMask = srcAccessMask;
    barrier.srcStageMask  = srcStage;

    barrier.dstAccessMask = dstAccessMask;
    barrier.dstStageMask  = dstStage;

    vk::DependencyInfo dep_info;
    dep_info.setImageMemoryBarriers(barrier);

    cmdBuffer.pipelineBarrier2(dep_info);
}

// Record a single-subresource image layout transition (color, mip 0, layer 0)
// with explicit src/dst access + stage masks. Shared by the transfer paths
// (upload/readback) and the renderer's attachment initialization. No ownership
// transfer — both queue families are Ignored (single-queue / same-family use).
inline void transitionImageLayout(const vk::Image&        image,
                                  vk::ImageLayout         oldLayout,
                                  vk::AccessFlags2        srcAccessMask,
                                  vk::PipelineStageFlags2 srcStage,
                                  vk::ImageLayout         newLayout,
                                  vk::AccessFlags2        dstAccessMask,
                                  vk::PipelineStageFlags2 dstStage,
                                  vk::CommandBuffer       cmdBuffer)
{
    transitionImageLayoutQFOT(image,
                              oldLayout,
                              srcAccessMask,
                              srcStage,
                              newLayout,
                              dstAccessMask,
                              dstStage,
                              vk::QueueFamilyIgnored,
                              vk::QueueFamilyIgnored,
                              cmdBuffer);
}

}}} // namespace caspar::accelerator::vulkan
