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

#include <common/utility/tweener.h>
#include <core/video_format.h>

#include <boost/array.hpp>
#include <type_traits>

namespace caspar { namespace core {

struct pixel_format_desc;
		
struct levels
{
	levels() 
		: min_input(0.0)
		, max_input(1.0)
		, gamma(1.0)
		, min_output(0.0)
		, max_output(1.0)
	{		
	}
	double min_input;
	double max_input;
	double gamma;
	double min_output;
	double max_output;
};

struct frame_transform 
{
public:

	frame_transform();

	double					volume;
	double					opacity;
	double					contrast;
	double					brightness;
	double					saturation;
	boost::array<double, 2>	fill_translation; 
	boost::array<double, 2>	fill_scale; 
	boost::array<double, 2>	clip_translation;  
	boost::array<double, 2>	clip_scale;  
	levels					levels;

	field_mode::type		field_mode;
	bool					is_key;
	bool					is_mix;
	
	frame_transform& frame_transform::operator*=(const frame_transform &other);
	frame_transform frame_transform::operator*(const frame_transform &other) const;
};

frame_transform tween(double time, const frame_transform& source, const frame_transform& dest, double duration, const tweener_t& tweener);

bool operator<(const frame_transform& lhs, const frame_transform& rhs);
bool operator==(const frame_transform& lhs, const frame_transform& rhs);
bool operator!=(const frame_transform& lhs, const frame_transform& rhs);

}}