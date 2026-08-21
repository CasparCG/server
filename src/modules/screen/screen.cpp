/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "screen.h"

#include "consumer/screen_consumer.h"

#include <core/consumer/frame_consumer.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>
#include <vector>

// The Vulkan screen consumer is non-Apple only in this phase (no macOS window
// backend yet); macOS keeps the GL consumer.
#if defined(ENABLE_VULKAN) && !defined(__APPLE__)
#include "vulkan_consumer/screen_consumer_vk.h"

#include <accelerator/vulkan/util/device.h>

#include <memory>
#endif

#ifdef ENABLE_VULKAN
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#endif

namespace caspar { namespace screen {

void init(const core::module_dependencies& dependencies)
{
#if defined(ENABLE_VULKAN) && !defined(__APPLE__)
    auto vk_device = std::dynamic_pointer_cast<accelerator::vulkan::device>(dependencies.accelerator_device);
#endif

    dependencies.consumer_registry->register_consumer_factory(
        L"Screen Consumer",
        [=](const std::vector<std::wstring>&                         params,
            const core::video_format_repository&                     format_repository,
            const std::vector<spl::shared_ptr<core::video_channel>>& channels,
            const core::channel_info& channel_info) -> spl::shared_ptr<core::frame_consumer> {
#if defined(ENABLE_VULKAN) && !defined(__APPLE__)
            if (vk_device)
                return vulkan::create_consumer(vk_device, params, format_repository, channels, channel_info);
#endif
            return create_consumer(params, format_repository, channels, channel_info);
        });

    dependencies.consumer_registry->register_preconfigured_consumer_factory(
        L"screen",
        [=](const boost::property_tree::wptree&                      ptree,
            const core::video_format_repository&                     format_repository,
            const std::vector<spl::shared_ptr<core::video_channel>>& channels,
            const core::channel_info& channel_info) -> spl::shared_ptr<core::frame_consumer> {
#if defined(ENABLE_VULKAN) && !defined(__APPLE__)
            if (vk_device)
                return vulkan::create_preconfigured_consumer(
                    vk_device, ptree, format_repository, channels, channel_info);
#endif
            return create_preconfigured_consumer(ptree, format_repository, channels, channel_info);
        });
}

#ifdef ENABLE_VULKAN
void register_vulkan_requirements(vkb::PhysicalDevice& pd)
{
    // Needed so the shared accelerator device can present to a window swapchain.
    pd.enable_extension_if_present(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}
#endif

}} // namespace caspar::screen
