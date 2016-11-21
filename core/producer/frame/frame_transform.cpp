/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../../stdafx.h"

#include "frame_transform.h"

#include <common/utility/assert.h>

#include <boost/thread/tss.hpp>

namespace caspar { namespace core {

corners::corners()
{
	ul[0] = 0.0;
	ul[1] = 0.0;
	ur[0] = 1.0;
	ur[1] = 0.0;
	lr[0] = 1.0;
	lr[1] = 1.0;
	ll[0] = 0.0;
	ll[1] = 1.0;
}

rectangle::rectangle()
{
	ul[0] = 0.0;
	ul[1] = 0.0;
	lr[0] = 1.0;
	lr[1] = 1.0;
}
		
frame_transform::frame_transform() 
	: volume(1.0)
	, opacity(1.0)
	, brightness(1.0)
	, contrast(1.0)
	, saturation(1.0)
	, angle(0.0)
	, field_mode(field_mode::progressive)
	, is_key(false)
	, is_mix(false)
	, is_paused(false)
{
	std::fill(anchor.begin(), anchor.end(), 0.0);
	std::fill(fill_translation.begin(), fill_translation.end(), 0.0);
	std::fill(fill_scale.begin(), fill_scale.end(), 1.0);
	std::fill(clip_translation.begin(), clip_translation.end(), 0.0);
	std::fill(clip_scale.begin(), clip_scale.end(), 1.0);
}

template<typename Rect>
void transform_rect(Rect& self, const Rect& other)
{
	self.ul[0] += other.ul[0];
	self.ul[1] += other.ul[1];
	self.lr[0] *= other.lr[0];
	self.lr[1] *= other.lr[1];
}

void transform_corners(corners& self, const corners& other)
{
	transform_rect(self, other);

	self.ur[0] *= other.ur[0];
	self.ur[1] += other.ur[1];
	self.ll[0] += other.ll[0];
	self.ll[1] *= other.ll[1];

	// TODO: figure out the math to compose perspective transforms correctly.
}

frame_transform& frame_transform::operator*=(const frame_transform &other)
{
	volume					*= other.volume;
	opacity					*= other.opacity;	
	brightness				*= other.brightness;
	contrast				*= other.contrast;
	saturation				*= other.saturation;

	// TODO: can this be done in any way without knowing the aspect ratio of the
	// actual video mode? Thread local to the rescue
	auto aspect_ratio = detail::get_current_aspect_ratio();
	aspect_ratio *= fill_scale[0] / fill_scale[1];
	boost::array<double, 2> rotated;
	auto orig_x = other.fill_translation[0];
	auto orig_y = other.fill_translation[1] / aspect_ratio;
	rotated[0] = orig_x * std::cos(angle) - orig_y * std::sin(angle);
	rotated[1] = orig_x * std::sin(angle) + orig_y * std::cos(angle);
	rotated[1] *= aspect_ratio;

	anchor[0]				+= other.anchor[0]*fill_scale[0];
	anchor[1]				+= other.anchor[1]*fill_scale[1];

	fill_translation[0]		+= rotated[0] * fill_scale[0];
	fill_translation[1]		+= rotated[1] * fill_scale[1];
	fill_scale[0]			*= other.fill_scale[0];
	fill_scale[1]			*= other.fill_scale[1];
	clip_translation[0]		+= other.clip_translation[0]*clip_scale[0];
	clip_translation[1]		+= other.clip_translation[1]*clip_scale[1];
	clip_scale[0]			*= other.clip_scale[0];
	clip_scale[1]			*= other.clip_scale[1];
	angle					+= other.angle;
	transform_rect(crop, other.crop);
	transform_corners(perspective, other.perspective);
	levels.min_input		 = std::max(levels.min_input,  other.levels.min_input);
	levels.max_input		 = std::min(levels.max_input,  other.levels.max_input);	
	levels.min_output		 = std::max(levels.min_output, other.levels.min_output);
	levels.max_output		 = std::min(levels.max_output, other.levels.max_output);
	levels.gamma			*= other.levels.gamma;
	field_mode				 = static_cast<field_mode::type>(field_mode & other.field_mode);
	is_key					|= other.is_key;
	is_mix					|= other.is_mix;
	is_paused				|= other.is_paused;

	return *this;
}

frame_transform frame_transform::operator*(const frame_transform &other) const
{
	return frame_transform(*this) *= other;
}

double do_tween(double time, double source, double dest, double duration, const tweener_t& tweener)
{
	return tweener(time, source, dest-source, duration);
};

template<typename Rect>
void do_tween_rectangle(const Rect& source, const Rect& dest, Rect& out, double time, double duration, const tweener_t& tweener)
{
	out.ul[0] = do_tween(time, source.ul[0], dest.ul[0], duration, tweener);
	out.ul[1] = do_tween(time, source.ul[1], dest.ul[1], duration, tweener);
	out.lr[0] = do_tween(time, source.lr[0], dest.lr[0], duration, tweener);
	out.lr[1] = do_tween(time, source.lr[1], dest.lr[1], duration, tweener);
}

void do_tween_corners(const corners& source, const corners& dest, corners& out, double time, double duration, const tweener_t& tweener)
{
	do_tween_rectangle(source, dest, out, time, duration, tweener);
	out.ur[0] = do_tween(time, source.ur[0], dest.ur[0], duration, tweener);
	out.ur[1] = do_tween(time, source.ur[1], dest.ur[1], duration, tweener);
	out.ll[0] = do_tween(time, source.ll[0], dest.ll[0], duration, tweener);
	out.ll[1] = do_tween(time, source.ll[1], dest.ll[1], duration, tweener);
};

frame_transform tween(double time, const frame_transform& source, const frame_transform& dest, double duration, const tweener_t& tweener)
{	
	frame_transform result;	
	result.volume				= do_tween(time, source.volume,					dest.volume,				duration, tweener);
	result.brightness			= do_tween(time, source.brightness,				dest.brightness,			duration, tweener);
	result.contrast				= do_tween(time, source.contrast,				dest.contrast,				duration, tweener);
	result.saturation			= do_tween(time, source.saturation,				dest.saturation,			duration, tweener);
	result.opacity				= do_tween(time, source.opacity,				dest.opacity,				duration, tweener);	
	result.anchor[0]			= do_tween(time, source.anchor[0],				dest.anchor[0],				duration, tweener), 
	result.anchor[1]			= do_tween(time, source.anchor[1],				dest.anchor[1],				duration, tweener);		
	result.fill_translation[0]	= do_tween(time, source.fill_translation[0],	dest.fill_translation[0],	duration, tweener), 
	result.fill_translation[1]	= do_tween(time, source.fill_translation[1],	dest.fill_translation[1],	duration, tweener);		
	result.fill_scale[0]		= do_tween(time, source.fill_scale[0],			dest.fill_scale[0],			duration, tweener), 
	result.fill_scale[1]		= do_tween(time, source.fill_scale[1],			dest.fill_scale[1],			duration, tweener);	
	result.clip_translation[0]	= do_tween(time, source.clip_translation[0],	dest.clip_translation[0],	duration, tweener), 
	result.clip_translation[1]	= do_tween(time, source.clip_translation[1],	dest.clip_translation[1],	duration, tweener);		
	result.clip_scale[0]		= do_tween(time, source.clip_scale[0],			dest.clip_scale[0],			duration, tweener), 
	result.clip_scale[1]		= do_tween(time, source.clip_scale[1],			dest.clip_scale[1],			duration, tweener);
	result.angle				= do_tween(time, source.angle,					dest.angle,					duration, tweener);	
	result.levels.max_input		= do_tween(time, source.levels.max_input,		dest.levels.max_input,		duration, tweener);
	result.levels.min_input		= do_tween(time, source.levels.min_input,		dest.levels.min_input,		duration, tweener);	
	result.levels.max_output	= do_tween(time, source.levels.max_output,		dest.levels.max_output,		duration, tweener);
	result.levels.min_output	= do_tween(time, source.levels.min_output,		dest.levels.min_output,		duration, tweener);
	result.levels.gamma			= do_tween(time, source.levels.gamma,			dest.levels.gamma,			duration, tweener);
	result.field_mode			= static_cast<field_mode::type>(source.field_mode & dest.field_mode);
	result.is_key				= source.is_key | dest.is_key;
	result.is_mix				= source.is_mix | dest.is_mix;
	result.is_paused			= source.is_paused | dest.is_paused;

	do_tween_rectangle(source.crop, dest.crop, result.crop, time, duration, tweener);
	do_tween_corners(source.perspective, dest.perspective, result.perspective, time, duration, tweener);

	return result;
}

bool operator<(const frame_transform& lhs, const frame_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(frame_transform)) < 0;
}

bool operator==(const frame_transform& lhs, const frame_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(frame_transform)) == 0;
}

bool operator!=(const frame_transform& lhs, const frame_transform& rhs)
{
	return !(lhs == rhs);
}

namespace detail {

double& get_thread_local_aspect_ratio()
{
	static boost::thread_specific_ptr<double> aspect_ratio;

	auto local = aspect_ratio.get();

	if (!local)
	{
		local = new double(1.0);
		aspect_ratio.reset(local);
	}

	return *local;
}

void set_current_aspect_ratio(double aspect_ratio)
{
	get_thread_local_aspect_ratio() = aspect_ratio;
}

double get_current_aspect_ratio()
{
	return get_thread_local_aspect_ratio();
}

}}}
