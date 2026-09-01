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
 * Author: Ole Kristensen, Den Frie Vilje ApS, ole@kristensen.name
 */

#pragma once

#include <common/memory.h>

#include <core/consumer/frame_consumer.h>
#include <core/video_channel.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <vector>

namespace caspar { namespace ffmpeg {

// Creates a consumer that renders to file as fast as the encoder allows,
// with no real-time synchronisation. The pipeline applies back-pressure
// automatically via a bounded frame queue.
//
// Config XML:
//   <offline>
//     <path>output.mov</path>
//     <args>-codec:v prores_ks -profile:v 3 -codec:a pcm_s24le</args>
//     <queue-depth>4</queue-depth>
//   </offline>
//
// AMCP:
//   ADD 1 OFFLINE output.mov -codec:v prores_ks -profile:v 3

spl::shared_ptr<core::frame_consumer>
create_offline_consumer(const std::vector<std::wstring>&                         params,
                        const core::video_format_repository&                     format_repository,
                        const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                        const core::channel_info&                                channel_info);

spl::shared_ptr<core::frame_consumer>
create_preconfigured_offline_consumer(const boost::property_tree::wptree&                      ptree,
                                      const core::video_format_repository&                     format_repository,
                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                      const core::channel_info&                                channel_info);

}} // namespace caspar::ffmpeg
