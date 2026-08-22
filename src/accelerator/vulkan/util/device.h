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

#include "queue_manager.h"

#include <accelerator/accelerator.h>
#include <common/array.h>
#include <common/bit_depth.h>
#include <core/frame/geometry.h>

#include <future>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

struct draw_params;

class image_kernel;
class vulkan_queue;
class transfer;

class device final
    : public std::enable_shared_from_this<device>
    , public accelerator_device
{
  public:
    explicit device(const std::vector<vulkan_requirements_fn>& requirements = {});
    ~device();

    device(const device&) = delete;

    device& operator=(const device&) = delete;

    vk::PhysicalDeviceMemoryProperties getMemoryProperties();
    vk::Device                         getVkDevice() const;
    vk::Instance                       instance() const;
    vk::PhysicalDevice                 physical_device() const;
    std::shared_ptr<vulkan_queue>      queue();
    // Hand out the queue dedicated to a kind of work (transfer/compute/video), so a
    // client (e.g. the screen consumer, hw decode) can run off the render queue.
    // Transfer/compute collapse to queue() on hardware without a dedicated family;
    // a video type returns nullptr when the hardware can't do it. Internally
    // synchronized and shared — no reclamation, exhaustion is impossible.
    std::shared_ptr<vulkan_queue> acquire_queue(queue_type type);
    class transfer&               transfer();

    // --- Interop surface: what an external Vulkan client needs to drive OUR device ---
    // (FFmpeg's AVVulkanDeviceContext is the one caller today: it decodes into this
    // device on the queues below, so nothing is copied across devices.) All handles
    // stay owned here — a client may use them, never destroy them.

    // The loader entry point the instance was built with. An external client must
    // resolve its own function pointers through this one, not through a second
    // loader instance.
    PFN_vkGetInstanceProcAddr instance_proc_addr() const;

    // The extensions actually enabled on the instance / device. External clients
    // key their feature detection off these lists rather than re-querying, so what
    // they believe is enabled matches what we asked for.
    const std::vector<std::string>& enabled_instance_extensions() const;
    const std::vector<std::string>& enabled_device_extensions() const;

    // The queue families we took queues from (see queue_family_usage), and the
    // reverse lookup an external submitter needs to find the vulkan_queue whose
    // lock it must hold while submitting on (family, index).
    std::vector<queue_family_usage> queue_families() const;
    std::shared_ptr<vulkan_queue>   queue_at(uint32_t family, uint32_t index) const;

    std::shared_ptr<class texture> create_texture(int width, int height, int stride, common::bit_depth depth);
    std::shared_ptr<class buffer>  create_buffer(int size, bool write);
    array<uint8_t>                 create_array(int size);

    std::wstring version() const;

    boost::property_tree::wptree info() const;
    std::future<void>            gc();

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
