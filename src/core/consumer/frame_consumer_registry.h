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

#include "../fwd.h"
#include "../monitor/monitor.h"

#include <common/bit_depth.h>
#include <common/memory.h>

#include <core/video_format.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <future>
#include <map>
#include <string>
#include <vector>

namespace caspar { namespace core {

void destroy_consumers_synchronously();

using consumer_factory_t =
    std::function<spl::shared_ptr<frame_consumer>(const std::vector<std::wstring>&              params,
                                                  const core::video_format_repository&          format_repository,
                                                  const spl::shared_ptr<core::frame_converter>& frame_converter,
                                                  const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                  common::bit_depth                                        depth)>;
using preconfigured_consumer_factory_t =
    std::function<spl::shared_ptr<frame_consumer>(const boost::property_tree::wptree&           element,
                                                  const core::video_format_repository&          format_repository,
                                                  const spl::shared_ptr<core::frame_converter>& frame_converter,
                                                  const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                  common::bit_depth                                        depth)>;

class frame_consumer_registry
{
  public:
    frame_consumer_registry();
    void register_consumer_factory(const std::wstring& name, const consumer_factory_t& factory);
    void register_preconfigured_consumer_factory(const std::wstring&                     element_name,
                                                 const preconfigured_consumer_factory_t& factory);
    spl::shared_ptr<frame_consumer> create_consumer(const std::vector<std::wstring>&              params,
                                                    const core::video_format_repository&          format_repository,
                                                    const spl::shared_ptr<core::frame_converter>& frame_converter,
                                                    const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                    common::bit_depth depth) const;
    spl::shared_ptr<frame_consumer> create_consumer(const std::wstring&                           element_name,
                                                    const boost::property_tree::wptree&           element,
                                                    const core::video_format_repository&          format_repository,
                                                    const spl::shared_ptr<core::frame_converter>& frame_converter,
                                                    const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                    common::bit_depth depth) const;

  private:
    std::vector<consumer_factory_t>                          consumer_factories_;
    std::map<std::wstring, preconfigured_consumer_factory_t> preconfigured_consumer_factories_;

    frame_consumer_registry(const frame_consumer_registry&)            = delete;
    frame_consumer_registry& operator=(const frame_consumer_registry&) = delete;
};

}} // namespace caspar::core
