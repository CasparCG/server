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

#include "completion_token.h"

#include <deque>
#include <functional>
#include <memory>
#include <mutex>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class vulkan_queue;

// A command pool + timeline semaphore + a deque of recycled one-time command
// buffers, bound to one vulkan_queue: record a one-time command buffer, submit
// it on the queue signalling the timeline, and return a completion_token. The
// timeline drives both buffer reclamation (reuse a buffer once the GPU passed
// its value) and the readback wait().
//
// Internally synchronized for the record path: one context is shared by every
// channel's transfer ops, so record_and_submit holds a mutex guarding the pool,
// the inflight ring and the timeline value. wait() is deliberately left
// lock-free — it only reads the semaphore via a self-contained completion_token,
// touching no mutable state, so a host readback can block without serializing
// other submitters. The owner must ensure the device is idle before destruction
// (the dtor does not waitIdle).
class command_context final
{
  public:
    command_context(vk::Device device, std::shared_ptr<vulkan_queue> queue);
    ~command_context();

    command_context(const command_context&)            = delete;
    command_context& operator=(const command_context&) = delete;

    // Reuse-or-allocate a one-time command buffer, begin/record(fn)/end, and
    // submit it on the bound queue signalling this context's timeline.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record);

    // Block until the token's value is reached (or timeout). True on success;
    // an empty token is already complete.
    bool wait(const completion_token& token, uint64_t timeout_ns = 1'000'000'000) const;

  private:
    struct inflight_command_buffer
    {
        vk::CommandBuffer cmd;
        uint64_t          value;
    };

    // Reuse the oldest retired command buffer, or allocate a fresh one.
    vk::CommandBuffer acquire_command_buffer();

    vk::Device                          device_;
    std::shared_ptr<vulkan_queue>       queue_;
    vk::CommandPool                     pool_;
    vk::Semaphore                       timeline_;
    uint64_t                            value_ = 0;
    std::deque<inflight_command_buffer> inflight_;
    std::mutex                          mutex_;
};

}}} // namespace caspar::accelerator::vulkan
