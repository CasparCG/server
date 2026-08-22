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

#pragma once

#ifdef ENABLE_VULKAN
namespace vkb {
struct PhysicalDevice;
} // namespace vkb

namespace caspar { namespace accelerator {
class accelerator_device;
}} // namespace caspar::accelerator
#endif

namespace caspar { namespace ffmpeg {

void                  init(const core::module_dependencies& dependencies);
void                  uninit();
std::shared_ptr<void> temporary_enable_quiet_logging_for_thread(bool enable);
void                  enable_quiet_logging_for_thread();
bool                  is_logging_quiet_for_thread();

#ifdef ENABLE_VULKAN
/// Asks the shared accelerator device for what FFmpeg's Vulkan decoder needs, so that decoding
/// can happen on the very device the mixer renders with. Registered with the accelerator before
/// the device is created (see the module's VULKAN_REQUIREMENTS_FUNCTION).
void register_vulkan_requirements(vkb::PhysicalDevice& pd);

/// Hands the module the accelerator's device, which the Vulkan video strategy wraps in an
/// AVHWDeviceContext on first use. Called from init().
void set_vulkan_accelerator_device(const std::shared_ptr<accelerator::accelerator_device>& accelerator_device);
#endif

}} // namespace caspar::ffmpeg
