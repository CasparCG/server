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

#include <common/memory.h>

#include <boost/property_tree/ptree.hpp>
#include <core/consumer/frame_consumer.h>

#include <string>
#include <vector>

namespace caspar { namespace image {

spl::shared_ptr<core::frame_consumer>
create_consumer(const std::vector<std::wstring>&                         params,
                const std::vector<spl::shared_ptr<core::video_channel>>& channels);

}} // namespace caspar::image
