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

#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class vulkan_queue;

// The kinds of work a caller can ask for a dedicated queue for. graphics is the
// primary render queue (always present); the rest are mapped to the most
// specialized family the hardware exposes for that capability, falling back to
// the graphics queue (transfer/compute, which graphics families always support)
// or to nothing at all (video, when no video family exists).
enum class queue_type
{
    graphics,
    transfer,
    compute,
    video_encode,
    video_decode,
};

const char* to_string(queue_type type);

// Picks which queue families to take queues from and owns the resulting queues.
//
// Two-phase, spanning device creation: queue count is frozen at vkCreateDevice.
// The ctor scans the families and resolves each queue_type to the best-matching
// family (graphics required; transfer/compute prefer a dedicated/async family;
// video maps to a video family when present). The device feeds queue_setup()
// into its custom queue setup, then calls create_queues() once the VkDevice
// exists. After that it hands queues out by type — primary() / acquire(type).
//
// One queue is created per resolved type where the family has spare capacity, so
// e.g. a dedicated transfer family and an async-compute family get their own
// queues. Types that resolve to the same family (or exceed its queueCount) share
// a queue — vulkan_queue is internally synchronized, so sharing can never
// exhaust and never races. No per-queue reclamation, no ref-counting.
class queue_manager final
{
  public:
    // Phase 1: scan + resolve each queue_type to a family. Logs the scan and the
    // resolved mapping. create_queues() must be called before any primary()/acquire().
    explicit queue_manager(vk::PhysicalDevice physical_device);

    queue_manager(const queue_manager&)            = delete;
    queue_manager& operator=(const queue_manager&) = delete;

    // The queues to create, as (family index, count) pairs — one queue per
    // distinct resolved family, with count > 1 where several types each took
    // their own queue from that family. Feed these to the device's custom queue
    // setup (one priority per queue).
    const std::vector<std::pair<uint32_t, uint32_t>>& queue_setup() const { return queue_setup_; }

    // Phase 2: the VkDevice has been created per queue_setup() — pull the handles
    // into vulkan_queues.
    void create_queues(vk::Device device);

    // The primary graphics queue; the render path.
    std::shared_ptr<vulkan_queue> primary() const;

    // The queue dedicated to the requested kind of work, or the graphics queue
    // when the kind has no dedicated family (transfer/compute always resolve to
    // something). Returns nullptr only for a video type the hardware can't do —
    // callers of the video types must check capability first.
    std::shared_ptr<vulkan_queue> acquire(queue_type type) const;

  private:
    static constexpr size_t type_count = 5; // keep in sync with queue_type

    // Per queue_type: the resolved family (UINT32_MAX = unavailable) and the
    // index into queues_ that serves it (SIZE_MAX = unavailable).
    std::array<uint32_t, type_count> type_family_;
    std::array<size_t, type_count>   type_queue_;

    // (family, queueIndexWithinFamily) for each entry in queues_, matching the
    // counts in queue_setup_.
    std::vector<std::pair<uint32_t, uint32_t>> queue_specs_;
    std::vector<std::pair<uint32_t, uint32_t>> queue_setup_;

    std::vector<std::shared_ptr<vulkan_queue>> queues_;
};

}}} // namespace caspar::accelerator::vulkan
