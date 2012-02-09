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

#include <common/tweener.h>
#include <core/video_format.h>

#include <boost/array.hpp>

namespace caspar { namespace core {
			
struct levels sealed
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

struct frame_transform sealed
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

	field_mode				field_mode;
	bool					is_key;
	bool					is_mix;
	
	frame_transform& frame_transform::operator*=(const frame_transform &other);
	frame_transform frame_transform::operator*(const frame_transform &other) const;

	static frame_transform tween(double time, const frame_transform& source, const frame_transform& dest, double duration, const tweener& tween);
};

bool operator==(const frame_transform& lhs, const frame_transform& rhs);
bool operator!=(const frame_transform& lhs, const frame_transform& rhs);

class tweened_transform
{
	frame_transform source_;
	frame_transform dest_;
	int duration_;
	int time_;
	tweener tweener_;
public:	
	tweened_transform()
		: duration_(0)
		, time_(0)
	{
	}

	tweened_transform(const frame_transform& source, const frame_transform& dest, int duration, const tweener& tween)
		: source_(source)
		, dest_(dest)
		, duration_(duration)
		, time_(0)
		, tweener_(tween)
	{
	}
	
	frame_transform fetch()
	{
		return time_ == duration_ ? dest_ : frame_transform::tween(static_cast<double>(time_), source_, dest_, static_cast<double>(duration_), tweener_);
	}

	frame_transform fetch_and_tick(int num)
	{						
		time_ = std::min(time_+num, duration_);
		return fetch();
	}
};

}}