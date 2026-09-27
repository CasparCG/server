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

#include "vulkan_queue.h"

namespace caspar { namespace accelerator { namespace vulkan {

vulkan_queue::vulkan_queue(vk::Queue queue, uint32_t family_index)
    : queue_(queue)
    , family_index_(family_index)
{
}

void vulkan_queue::submit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.submit(submits, fence);
}

}}} // namespace caspar::accelerator::vulkan
