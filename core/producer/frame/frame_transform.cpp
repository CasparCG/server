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

#include "../../stdafx.h"

#include "frame_transform.h"

#include <common/utility/assert.h>

namespace caspar { namespace core {
		
frame_transform::frame_transform() 
	: volume(1.0)
	, opacity(1.0)
	, brightness(1.0)
	, contrast(1.0)
	, saturation(1.0)
	, field_mode(field_mode::progressive)
	, is_key(false)
	, is_mix(false)
{
	std::fill(fill_translation.begin(), fill_translation.end(), 0.0);
	std::fill(fill_scale.begin(), fill_scale.end(), 1.0);
	std::fill(clip_translation.begin(), clip_translation.end(), 0.0);
	std::fill(clip_scale.begin(), clip_scale.end(), 1.0);
}

frame_transform& frame_transform::operator*=(const frame_transform &other)
{
	volume					*= other.volume;
	opacity					*= other.opacity;	
	brightness				*= other.brightness;
	contrast				*= other.contrast;
	saturation				*= other.saturation;
	fill_translation[0]		+= other.fill_translation[0]*fill_scale[0];
	fill_translation[1]		+= other.fill_translation[1]*fill_scale[1];
	fill_scale[0]			*= other.fill_scale[0];
	fill_scale[1]			*= other.fill_scale[1];
	clip_translation[0]		+= other.clip_translation[0]*clip_scale[0];
	clip_translation[1]		+= other.clip_translation[1]*clip_scale[1];
	clip_scale[0]			*= other.clip_scale[0];
	clip_scale[1]			*= other.clip_scale[1];
	levels.min_input		 = std::max(levels.min_input,  other.levels.min_input);
	levels.max_input		 = std::min(levels.max_input,  other.levels.max_input);	
	levels.min_output		 = std::max(levels.min_output, other.levels.min_output);
	levels.max_output		 = std::min(levels.max_output, other.levels.max_output);
	levels.gamma			*= other.levels.gamma;
	field_mode				 = static_cast<field_mode::type>(field_mode & other.field_mode);
	is_key					|= other.is_key;
	is_mix					|= other.is_mix;
	return *this;
}

frame_transform frame_transform::operator*(const frame_transform &other) const
{
	return frame_transform(*this) *= other;
}

frame_transform tween(double time, const frame_transform& source, const frame_transform& dest, double duration, const tweener_t& tweener)
{	
	auto do_tween = [](double time, double source, double dest, double duration, const tweener_t& tweener)
	{
		return tweener(time, source, dest-source, duration);
	};
	
	frame_transform result;	
	result.volume				= do_tween(time, source.volume,					dest.volume,				duration, tweener);
	result.brightness			= do_tween(time, source.brightness,				dest.brightness,			duration, tweener);
	result.contrast				= do_tween(time, source.contrast,				dest.contrast,				duration, tweener);
	result.saturation			= do_tween(time, source.saturation,				dest.saturation,			duration, tweener);
	result.opacity				= do_tween(time, source.opacity,				dest.opacity,				duration, tweener);	
	result.fill_translation[0]	= do_tween(time, source.fill_translation[0],	dest.fill_translation[0],	duration, tweener), 
	result.fill_translation[1]	= do_tween(time, source.fill_translation[1],	dest.fill_translation[1],	duration, tweener);		
	result.fill_scale[0]		= do_tween(time, source.fill_scale[0],			dest.fill_scale[0],			duration, tweener), 
	result.fill_scale[1]		= do_tween(time, source.fill_scale[1],			dest.fill_scale[1],			duration, tweener);	
	result.clip_translation[0]	= do_tween(time, source.clip_translation[0],	dest.clip_translation[0],	duration, tweener), 
	result.clip_translation[1]	= do_tween(time, source.clip_translation[1],	dest.clip_translation[1],	duration, tweener);		
	result.clip_scale[0]		= do_tween(time, source.clip_scale[0],			dest.clip_scale[0],			duration, tweener), 
	result.clip_scale[1]		= do_tween(time, source.clip_scale[1],			dest.clip_scale[1],			duration, tweener);
	result.levels.max_input		= do_tween(time, source.levels.max_input,		dest.levels.max_input,		duration, tweener);
	result.levels.min_input		= do_tween(time, source.levels.min_input,		dest.levels.min_input,		duration, tweener);	
	result.levels.max_output	= do_tween(time, source.levels.max_output,		dest.levels.max_output,		duration, tweener);
	result.levels.min_output	= do_tween(time, source.levels.min_output,		dest.levels.min_output,		duration, tweener);
	result.levels.gamma			= do_tween(time, source.levels.gamma,			dest.levels.gamma,			duration, tweener);
	result.field_mode			= static_cast<field_mode::type>(source.field_mode & dest.field_mode);
	result.is_key				= source.is_key | dest.is_key;
	result.is_mix				= source.is_mix | dest.is_mix;
	
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

}}