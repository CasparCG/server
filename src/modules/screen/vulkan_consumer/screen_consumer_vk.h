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
 */

#pragma once

#include <common/memory.h>

#include <core/fwd.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <memory>
#include <string>
#include <vector>

namespace caspar { namespace accelerator { namespace vulkan {
class device;
}}} // namespace caspar::accelerator::vulkan

namespace caspar { namespace screen { namespace vulkan {

// Vulkan-based screen consumer. Presents to a GLFW window using the shared
// accelerator Vulkan device passed in from the module dependencies.
spl::shared_ptr<core::frame_consumer> create_consumer(const std::shared_ptr<accelerator::vulkan::device>& device,
                                                      const std::vector<std::wstring>&                    params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info);

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const std::shared_ptr<accelerator::vulkan::device>&      device,
                              const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info);

}}} // namespace caspar::screen::vulkan
