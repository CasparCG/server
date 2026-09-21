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

#include "descriptor_pool.h"

namespace caspar { namespace accelerator { namespace vulkan {

descriptor_pool::descriptor_pool(vk::Device                                 device,
                                 vk::DescriptorSetLayout                    layout,
                                 const std::vector<vk::DescriptorPoolSize>& sizes_per_set)
    : device_(device)
    , layout_(layout)
    , sizes_per_set_(sizes_per_set)
{
}

descriptor_pool::~descriptor_pool() { destroy(); }

void descriptor_pool::destroy()
{
    if (pool_) {
        device_.destroyDescriptorPool(pool_);
        pool_ = nullptr;
    }
    capacity_ = 0;
}

vk::DescriptorPool descriptor_pool::create_pool(uint32_t max_sets) const
{
    auto sizes = sizes_per_set_;
    for (auto& size : sizes)
        size.descriptorCount *= max_sets;

    vk::DescriptorPoolCreateInfo info{};
    info.maxSets = max_sets;
    info.setPoolSizes(sizes);
    return device_.createDescriptorPool(info);
}

std::vector<vk::DescriptorSet> descriptor_pool::allocate(uint32_t count)
{
    if (count == 0)
        return {};

    if (count > capacity_) {
        // Grow to the new high-water mark; the old pool's sets are from an
        // already-completed frame (slot token waited), so it is safe to drop.
        destroy();
        pool_     = create_pool(count);
        capacity_ = count;
    } else {
        device_.resetDescriptorPool(pool_);
    }

    std::vector<vk::DescriptorSetLayout> layouts(count, layout_);
    vk::DescriptorSetAllocateInfo        info{};
    info.descriptorPool = pool_;
    info.setSetLayouts(layouts);
    return device_.allocateDescriptorSets(info);
}

}}} // namespace caspar::accelerator::vulkan
