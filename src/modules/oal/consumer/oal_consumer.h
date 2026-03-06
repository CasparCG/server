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

#include <boost/property_tree/ptree_fwd.hpp>
#include <common/bit_depth.h>
#include <common/memory.h>
#include <core/fwd.h>
#include <functional>
#include <vector>

namespace caspar { namespace oal {

// Forward declaration of oal_consumer class
class oal_consumer;

// Interface for video-scheduled audio consumers
class video_scheduled_audio_consumer
{
  public:
    virtual ~video_scheduled_audio_consumer()           = default;
    virtual void schedule_audio_for_frame(int64_t                     frame_number,
                                          const std::vector<int32_t>& samples,
                                          int                         sample_rate,
                                          int                         channels) = 0;
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info);

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info);

}} // namespace caspar::oal
