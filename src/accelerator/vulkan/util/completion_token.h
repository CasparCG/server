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

#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

// Handle to a submitted batch: a value on a command_context's timeline
// semaphore. wait() on it to observe completion. A default-constructed token is
// "nothing submitted" and is treated as already complete.
struct completion_token
{
    vk::Semaphore timeline{};
    uint64_t      value = 0;

    explicit operator bool() const { return timeline && value > 0; }
};

}}} // namespace caspar::accelerator::vulkan
