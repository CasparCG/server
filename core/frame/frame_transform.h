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
			
struct levels final
{
	double min_input	= 0.0;
	double max_input	= 1.0;
	double gamma		= 1.0;
	double min_output	= 0.0;
	double max_output	= 1.0;
};

struct corners final
{
	boost::array<double, 2> ul = boost::array<double, 2> { { 0.0, 0.0 } };
	boost::array<double, 2> ur = boost::array<double, 2> { { 1.0, 0.0 } };
	boost::array<double, 2> lr = boost::array<double, 2> { { 1.0, 1.0 } };
	boost::array<double, 2> ll = boost::array<double, 2> { { 0.0, 1.0 } };
};

struct rectangle final
{
	boost::array<double, 2> ul = boost::array<double, 2> { { 0.0, 0.0 } };
	boost::array<double, 2> lr = boost::array<double, 2> { { 1.0, 1.0 } };
};

struct image_transform final
{
	double					opacity				= 1.0;
	double					contrast			= 1.0;
	double					brightness			= 1.0;
	double					saturation			= 1.0;

	// A bug in VS 2013 prevents us from writing:
	// boost::array<double, 2> fill_translation = { { 0.0, 0.0 } };
	// See http://blogs.msdn.com/b/vcblog/archive/2014/08/19/the-future-of-non-static-data-member-initialization.aspx
	boost::array<double, 2>	anchor				= boost::array<double, 2> { { 0.0, 0.0 } };
	boost::array<double, 2>	fill_translation	= boost::array<double, 2> { { 0.0, 0.0 } };
	boost::array<double, 2>	fill_scale			= boost::array<double, 2> { { 1.0, 1.0 } };
	boost::array<double, 2>	clip_translation	= boost::array<double, 2> { { 0.0, 0.0 } };
	boost::array<double, 2>	clip_scale			= boost::array<double, 2> { { 1.0, 1.0 } };
	double					angle				= 0.0;
	rectangle				crop;
	corners					perspective;
	core::levels			levels;

	core::field_mode		field_mode			= core::field_mode::progressive;
	bool					is_key				= false;
	bool					is_mix				= false;
	bool					is_still			= false;
	bool					use_mipmap			= false;
	
	image_transform& operator*=(const image_transform &other);
	image_transform operator*(const image_transform &other) const;

	static image_transform tween(double time, const image_transform& source, const image_transform& dest, double duration, const tweener& tween);
};

bool operator==(const image_transform& lhs, const image_transform& rhs);
bool operator!=(const image_transform& lhs, const image_transform& rhs);

struct audio_transform final
{
	double	volume		= 1.0;
	bool	is_still	= false;
	
	audio_transform& operator*=(const audio_transform &other);
	audio_transform operator*(const audio_transform &other) const;

	static audio_transform tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener& tween);
};

bool operator==(const audio_transform& lhs, const audio_transform& rhs);
bool operator!=(const audio_transform& lhs, const audio_transform& rhs);

//__declspec(align(16)) 
struct frame_transform final
{
public:
	frame_transform();
	
	core::image_transform image_transform;
	core::audio_transform audio_transform;

	//char padding[(sizeof(core::image_transform) + sizeof(core::audio_transform)) % 16];
	
	frame_transform& operator*=(const frame_transform &other);
	frame_transform operator*(const frame_transform &other) const;

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

namespace detail {

void set_current_aspect_ratio(double aspect_ratio);
double get_current_aspect_ratio();

}}}
