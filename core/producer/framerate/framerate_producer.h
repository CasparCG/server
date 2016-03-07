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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include "../../fwd.h"

#include <common/memory.h>

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

#include <boost/rational.hpp>

namespace caspar { namespace core {

spl::shared_ptr<frame_producer> create_framerate_producer(
		spl::shared_ptr<frame_producer> source,
		boost::rational<int> source_framerate,
		boost::rational<int> destination_framerate,
		field_mode destination_fieldmode,
		std::vector<int> destination_audio_cadence);

}}
