/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
* Copyright (c) 2013 Technical University of Lodz Multimedia Centre <office@cm.p.lodz.pl>
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
*		  Jan Starzak, jan@ministryofgoodsteps.com
*/

#pragma once

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

namespace caspar { namespace replay {

safe_ptr<core::frame_producer> create_producer(
		const safe_ptr<core::frame_factory>& frame_factory,
		const std::vector<std::wstring>& params,
		const std::vector<std::wstring>& original_case_params);

}}