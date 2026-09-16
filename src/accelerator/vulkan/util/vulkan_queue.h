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
#include <mutex>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

// A single VkQueue together with the family it was created from. Mirrors
// vk::Queue::submit so call sites stop passing raw handles around.
//
// Internally synchronized: vkQueueSubmit requires external synchronization of
// the VkQueue, and this one queue is shared by every submitter (the transfer
// command_context and the render ring, across all channel threads). submit()
// holds a mutex so those submits serialize. This is the queue's own lock; the
// transfer command_context guards its own pool/ring with a separate lock (see
// command_context).
class vulkan_queue final
{
  public:
    vulkan_queue(vk::Queue queue, uint32_t family_index);

    vulkan_queue(const vulkan_queue&)            = delete;
    vulkan_queue& operator=(const vulkan_queue&) = delete;

    void submit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence = {});

    uint32_t family_index() const { return family_index_; }

    // The raw queue handle, for submitters that need vkQueueSubmit2 (binary +
    // timeline semaphore mix) or vkQueuePresentKHR, which submit() does not wrap.
    // Such callers must hold scoped_lock() to honour the queue's external sync.
    vk::Queue vk_queue() const { return queue_; }

    // Hold this while submitting through vk_queue() directly, so those submits
    // serialize against the ones going through submit().
    std::unique_lock<std::mutex> scoped_lock() { return std::unique_lock<std::mutex>(mutex_); }

  private:
    vk::Queue  queue_;
    uint32_t   family_index_;
    std::mutex mutex_;
};

}}} // namespace caspar::accelerator::vulkan
