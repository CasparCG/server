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

#include "queue_manager.h"

#include "vulkan_queue.h"

#include <common/except.h>
#include <common/log.h>

#include <algorithm>
#include <map>

namespace caspar { namespace accelerator { namespace vulkan {

namespace {

// The capability bits we score families on. A family supporting fewer of these
// is more specialized (a dedicated transfer/video family), which is exactly what
// we want to offload onto when one exists.
constexpr vk::QueueFlags capability_bits()
{
    return vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer |
           vk::QueueFlagBits::eVideoEncodeKHR | vk::QueueFlagBits::eVideoDecodeKHR;
}

int count_capabilities(vk::QueueFlags flags)
{
    int n = 0;
    for (auto bit : {vk::QueueFlagBits::eGraphics,
                     vk::QueueFlagBits::eCompute,
                     vk::QueueFlagBits::eTransfer,
                     vk::QueueFlagBits::eVideoEncodeKHR,
                     vk::QueueFlagBits::eVideoDecodeKHR}) {
        if (flags & bit)
            ++n;
    }
    return n;
}

// The most specialized family supporting `required`, preferring one distinct
// from the graphics family so the work actually runs off the render queue.
// UINT32_MAX if no family supports the capability.
uint32_t best_family(const std::vector<vk::QueueFamilyProperties>& family_props,
                     vk::QueueFlagBits                             required,
                     uint32_t                                      graphics_family)
{
    uint32_t best      = UINT32_MAX;
    int      best_caps = 0;
    for (uint32_t i = 0; i < family_props.size(); ++i) {
        auto flags = family_props[i].queueFlags;
        if (!(flags & required))
            continue;
        int caps = count_capabilities(flags & capability_bits());
        // Prefer fewer capabilities (more dedicated); on a tie, prefer a family
        // other than the graphics one.
        bool better = best == UINT32_MAX || caps < best_caps ||
                      (caps == best_caps && best == graphics_family && i != graphics_family);
        if (better) {
            best      = i;
            best_caps = caps;
        }
    }
    return best;
}

} // namespace

const char* to_string(queue_type type)
{
    switch (type) {
        case queue_type::graphics:
            return "graphics";
        case queue_type::transfer:
            return "transfer";
        case queue_type::compute:
            return "compute";
        case queue_type::video_encode:
            return "video_encode";
        case queue_type::video_decode:
            return "video_decode";
    }
    return "unknown";
}

queue_manager::queue_manager(vk::PhysicalDevice physical_device)
{
    auto family_props = physical_device.getQueueFamilyProperties();

    uint32_t graphics_family = best_family(family_props, vk::QueueFlagBits::eGraphics, UINT32_MAX);
    if (graphics_family == UINT32_MAX) {
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("No graphics-capable Vulkan queue family found"));
    }

    // Resolve each queue_type to a family. Transfer and compute fall back to the
    // graphics family (which always supports transfer and, in practice, compute);
    // video falls back to nothing — that hardware simply can't do it.
    type_family_[static_cast<size_t>(queue_type::graphics)] = graphics_family;
    type_family_[static_cast<size_t>(queue_type::transfer)] =
        best_family(family_props, vk::QueueFlagBits::eTransfer, graphics_family);
    type_family_[static_cast<size_t>(queue_type::compute)] =
        best_family(family_props, vk::QueueFlagBits::eCompute, graphics_family);
    type_family_[static_cast<size_t>(queue_type::video_encode)] =
        best_family(family_props, vk::QueueFlagBits::eVideoEncodeKHR, graphics_family);
    type_family_[static_cast<size_t>(queue_type::video_decode)] =
        best_family(family_props, vk::QueueFlagBits::eVideoDecodeKHR, graphics_family);

    if (type_family_[static_cast<size_t>(queue_type::transfer)] == UINT32_MAX)
        type_family_[static_cast<size_t>(queue_type::transfer)] = graphics_family;
    if (type_family_[static_cast<size_t>(queue_type::compute)] == UINT32_MAX)
        type_family_[static_cast<size_t>(queue_type::compute)] = graphics_family;

    // Lay out the queues: one per resolved type where the family has spare
    // capacity, sharing within a family once its queueCount is used up. Graphics
    // is laid out first so the primary queue is queues_[0].
    std::map<uint32_t, uint32_t> used_per_family; // family -> queues already taken
    auto                         lay_out = [&](queue_type type) {
        auto t = static_cast<size_t>(type);
        auto f = type_family_[t];
        if (f == UINT32_MAX) {
            type_queue_[t] = SIZE_MAX;
            return;
        }
        uint32_t taken = used_per_family[f];
        if (taken < family_props[f].queueCount) {
            // Room for a dedicated queue on this family.
            used_per_family[f] = taken + 1;
            type_queue_[t]     = queue_specs_.size();
            queue_specs_.emplace_back(f, taken);
        } else {
            // Family exhausted — share its first queue.
            for (size_t q = 0; q < queue_specs_.size(); ++q) {
                if (queue_specs_[q].first == f) {
                    type_queue_[t] = q;
                    break;
                }
            }
        }
    };
    lay_out(queue_type::graphics);
    lay_out(queue_type::transfer);
    lay_out(queue_type::compute);
    lay_out(queue_type::video_encode);
    lay_out(queue_type::video_decode);

    // Collapse the per-queue specs into (family, count) for the device builder.
    // vkCreateDevice requires each family to appear at most once, and a family
    // can show up non-consecutively above (e.g. transfer takes a dedicated
    // family while compute falls back to the graphics one), so aggregate by
    // family rather than merging neighbours.
    for (const auto& spec : queue_specs_) {
        auto it = std::find_if(
            queue_setup_.begin(), queue_setup_.end(), [&](const auto& e) { return e.first == spec.first; });
        if (it != queue_setup_.end())
            ++it->second;
        else
            queue_setup_.emplace_back(spec.first, 1u);
    }

    CASPAR_LOG(info) << "Vulkan: " << family_props.size() << " queue families available, creating "
                     << queue_specs_.size() << " queues.";
    for (uint32_t i = 0; i < family_props.size(); ++i) {
        CASPAR_LOG(info) << "  queue family " << i << ": queueCount=" << family_props[i].queueCount
                         << " flags=" << vk::to_string(family_props[i].queueFlags).c_str();
    }
    for (size_t t = 0; t < type_count; ++t) {
        auto type = static_cast<queue_type>(t);
        if (type_family_[t] == UINT32_MAX)
            CASPAR_LOG(info) << "  " << to_string(type) << " queue: unavailable";
        else
            CASPAR_LOG(info) << "  " << to_string(type) << " queue: family " << type_family_[t];
    }
}

void queue_manager::create_queues(vk::Device device)
{
    for (const auto& spec : queue_specs_) {
        queues_.push_back(std::make_shared<vulkan_queue>(device.getQueue(spec.first, spec.second), spec.first));
    }
}

std::shared_ptr<vulkan_queue> queue_manager::primary() const { return acquire(queue_type::graphics); }

std::shared_ptr<vulkan_queue> queue_manager::acquire(queue_type type) const
{
    auto q = type_queue_[static_cast<size_t>(type)];
    if (q == SIZE_MAX)
        return nullptr;
    return queues_.at(q);
}

std::vector<queue_family_usage> queue_manager::families() const
{
    // What each kind of work needs a family to advertise. Video maps to its own
    // bit only: a video family that also reports transfer must not be picked up
    // by someone looking for a transfer queue.
    auto usage_of = [](queue_type type) -> vk::QueueFlags {
        switch (type) {
            case queue_type::graphics:
                return vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
            case queue_type::compute:
                return vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
            case queue_type::transfer:
                return vk::QueueFlagBits::eTransfer;
            case queue_type::video_encode:
                return vk::QueueFlagBits::eVideoEncodeKHR;
            case queue_type::video_decode:
                return vk::QueueFlagBits::eVideoDecodeKHR;
        }
        return {};
    };

    std::vector<queue_family_usage> result;
    auto                            entry_for = [&](uint32_t family) -> queue_family_usage& {
        auto it = std::find_if(result.begin(), result.end(), [&](const auto& e) { return e.index == family; });
        if (it != result.end())
            return *it;
        queue_family_usage e;
        e.index = family;
        e.count = static_cast<uint32_t>(
            std::count_if(queue_specs_.begin(), queue_specs_.end(), [&](const auto& s) { return s.first == family; }));
        result.push_back(e);
        return result.back();
    };

    // Specialized types first, graphics last, so a consumer that takes the first
    // family matching a capability prefers the dedicated one.
    for (auto type : {queue_type::transfer,
                      queue_type::compute,
                      queue_type::video_decode,
                      queue_type::video_encode,
                      queue_type::graphics}) {
        auto family = type_family_[static_cast<size_t>(type)];
        if (family == UINT32_MAX)
            continue;
        entry_for(family).usage |= usage_of(type);
    }

    return result;
}

std::shared_ptr<vulkan_queue> queue_manager::queue_at(uint32_t family, uint32_t index) const
{
    for (size_t q = 0; q < queue_specs_.size(); ++q) {
        if (queue_specs_[q].first == family && queue_specs_[q].second == index)
            return q < queues_.size() ? queues_[q] : nullptr;
    }
    return nullptr;
}

}}} // namespace caspar::accelerator::vulkan
