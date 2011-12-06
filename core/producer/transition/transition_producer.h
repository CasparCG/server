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

#include "../frame_producer.h"
#include "../../video_format.h"

#include <common/utility/tweener.h>

#include <string>
#include <memory>

namespace caspar { namespace core {

struct transition
{
	enum type
	{

		cut = 1,
		mix,
		push,
		slide,
		wipe
	};
};

struct transition_direction
{
	enum type
	{
		from_left = 1,
		from_right
	};
};

struct transition_info
{
	transition_info() 
		: type(transition::cut)
		, duration(0)
		, direction(transition_direction::from_left)
		, tweener(get_tweener(L"linear")){}
		
	size_t						duration;
	transition_direction::type	direction;
	transition::type			type;
	tweener_t					tweener;
};

safe_ptr<frame_producer> create_transition_producer(const field_mode::type& mode, const safe_ptr<frame_producer>& destination, const transition_info& info);

}}