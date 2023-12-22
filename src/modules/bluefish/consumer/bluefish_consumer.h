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
 *         James Wise,  james.wise@bluefish444.com
 */

#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <common/memory.h>
#include <core/fwd.h>

#include <string>
#include <vector>

namespace caspar { namespace bluefish {

spl::shared_ptr<core::frame_consumer>
create_consumer(const std::vector<std::wstring>&                         params,
                const core::video_format_repository&                     format_repository,
                const std::vector<spl::shared_ptr<core::video_channel>>& channels);

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels);

}} // namespace caspar::bluefish
