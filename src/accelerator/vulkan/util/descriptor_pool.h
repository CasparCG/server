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

#include <vector>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

// A resettable VkDescriptorPool sized to one frame's exact need. Owned per
// render-frame slot (alongside that slot's vertex buffer); since the slot's
// completion_token is waited before reuse (image_kernel::create_renderpass), a
// reset here can never recycle a set still read by an in-flight submission.
//
// allocate(count) frees the previous frame's sets and hands back exactly `count`
// fresh ones. The underlying VkDescriptorPool grows monotonically to the largest
// count ever requested (high-water mark) and is reused (reset, not recreated) on
// every smaller frame, so steady state is allocation-free churn. Not internally
// synchronized: one instance lives on its kernel's (channel's) thread.
class descriptor_pool final
{
  public:
    descriptor_pool(vk::Device                                 device,
                    vk::DescriptorSetLayout                    layout,
                    const std::vector<vk::DescriptorPoolSize>& sizes_per_set);
    ~descriptor_pool();

    descriptor_pool(const descriptor_pool&)            = delete;
    descriptor_pool& operator=(const descriptor_pool&) = delete;

    // Reset the pool (freeing the previous frame's sets) and allocate `count`
    // fresh sets of the bound layout. count == 0 returns empty without touching
    // the pool.
    std::vector<vk::DescriptorSet> allocate(uint32_t count);

  private:
    vk::DescriptorPool create_pool(uint32_t max_sets) const;
    void               destroy();

    vk::Device                          device_;
    vk::DescriptorSetLayout             layout_;
    std::vector<vk::DescriptorPoolSize> sizes_per_set_;
    vk::DescriptorPool                  pool_     = nullptr;
    uint32_t                            capacity_ = 0;
};

}}} // namespace caspar::accelerator::vulkan
