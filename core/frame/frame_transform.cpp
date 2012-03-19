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

#include "../stdafx.h"

#include "frame_transform.h"

#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/fill.hpp>

namespace caspar { namespace core {
		
// image_transform

image_transform::image_transform() 
	: opacity(1.0)
	, brightness(1.0)
	, contrast(1.0)
	, saturation(1.0)
	, field_mode(field_mode::progressive)
	, is_key(false)
	, is_mix(false)
	, is_still(false)
{
	boost::range::fill(fill_translation, 0.0);
	boost::range::fill(fill_scale, 1.0);
	boost::range::fill(clip_translation, 0.0);
	boost::range::fill(clip_scale, 1.0);
}

image_transform& image_transform::operator*=(const image_transform &other)
{
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
	field_mode				 = static_cast<core::field_mode>(field_mode & other.field_mode);
	is_key					|= other.is_key;
	is_mix					|= other.is_mix;
	is_still				|= other.is_still;
	return *this;
}

image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

image_transform image_transform::tween(double time, const image_transform& source, const image_transform& dest, double duration, const tweener& tween)
{	
	auto do_tween = [](double time, double source, double dest, double duration, const tweener& tween)
	{
		return tween(time, source, dest-source, duration);
	};
	
	image_transform result;	
	result.brightness			= do_tween(time, source.brightness,				dest.brightness,			duration, tween);
	result.contrast				= do_tween(time, source.contrast,				dest.contrast,				duration, tween);
	result.saturation			= do_tween(time, source.saturation,				dest.saturation,			duration, tween);
	result.opacity				= do_tween(time, source.opacity,				dest.opacity,				duration, tween);	
	result.fill_translation[0]	= do_tween(time, source.fill_translation[0],	dest.fill_translation[0],	duration, tween), 
	result.fill_translation[1]	= do_tween(time, source.fill_translation[1],	dest.fill_translation[1],	duration, tween);		
	result.fill_scale[0]		= do_tween(time, source.fill_scale[0],			dest.fill_scale[0],			duration, tween), 
	result.fill_scale[1]		= do_tween(time, source.fill_scale[1],			dest.fill_scale[1],			duration, tween);	
	result.clip_translation[0]	= do_tween(time, source.clip_translation[0],	dest.clip_translation[0],	duration, tween), 
	result.clip_translation[1]	= do_tween(time, source.clip_translation[1],	dest.clip_translation[1],	duration, tween);		
	result.clip_scale[0]		= do_tween(time, source.clip_scale[0],			dest.clip_scale[0],			duration, tween), 
	result.clip_scale[1]		= do_tween(time, source.clip_scale[1],			dest.clip_scale[1],			duration, tween);
	result.levels.max_input		= do_tween(time, source.levels.max_input,		dest.levels.max_input,		duration, tween);
	result.levels.min_input		= do_tween(time, source.levels.min_input,		dest.levels.min_input,		duration, tween);	
	result.levels.max_output	= do_tween(time, source.levels.max_output,		dest.levels.max_output,		duration, tween);
	result.levels.min_output	= do_tween(time, source.levels.min_output,		dest.levels.min_output,		duration, tween);
	result.levels.gamma			= do_tween(time, source.levels.gamma,			dest.levels.gamma,			duration, tween);
	result.field_mode			= source.field_mode & dest.field_mode;
	result.is_key				= source.is_key | dest.is_key;
	result.is_mix				= source.is_mix | dest.is_mix;
	result.is_still				= source.is_still | dest.is_still;
	
	return result;
}

bool operator==(const image_transform& lhs, const image_transform& rhs)
{
	auto eq = [](double lhs, double rhs)
	{
		return std::abs(lhs - rhs) < 5e-8;
	};

	return 
		eq(lhs.opacity, rhs.opacity) &&
		eq(lhs.contrast, rhs.contrast) &&
		eq(lhs.brightness, rhs.brightness) &&
		eq(lhs.saturation, rhs.saturation) &&
		boost::range::equal(lhs.fill_translation, rhs.fill_translation, eq) &&
		boost::range::equal(lhs.fill_scale, rhs.fill_scale, eq) &&
		boost::range::equal(lhs.clip_translation, rhs.clip_translation, eq) &&
		boost::range::equal(lhs.clip_scale, rhs.clip_scale, eq) &&
		lhs.field_mode == rhs.field_mode &&
		lhs.is_key == rhs.is_key &&
		lhs.is_mix == rhs.is_mix &&
		lhs.is_still == rhs.is_still;
}

bool operator!=(const image_transform& lhs, const image_transform& rhs)
{
	return !(lhs == rhs);
}

// audio_transform
		
audio_transform::audio_transform() 
	: volume(1.0)
	, is_still(false)
{
}

audio_transform& audio_transform::operator*=(const audio_transform &other)
{
	volume	 *= other.volume;	
	is_still |= other.is_still;
	return *this;
}

audio_transform audio_transform::operator*(const audio_transform &other) const
{
	return audio_transform(*this) *= other;
}

audio_transform audio_transform::tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener& tween)
{	
	auto do_tween = [](double time, double source, double dest, double duration, const tweener& tween)
	{
		return tween(time, source, dest-source, duration);
	};
	
	audio_transform result;	
	result.is_still			= source.is_still | dest.is_still;
	result.volume			= do_tween(time, source.volume,				dest.volume,			duration, tween);
	
	return result;
}

bool operator==(const audio_transform& lhs, const audio_transform& rhs)
{
	auto eq = [](double lhs, double rhs)
	{
		return std::abs(lhs - rhs) < 5e-8;
	};

	return eq(lhs.volume, rhs.volume) && lhs.is_still == rhs.is_still;
}

bool operator!=(const audio_transform& lhs, const audio_transform& rhs)
{
	return !(lhs == rhs);
}

// frame_transform
frame_transform::frame_transform()
{
}

frame_transform& frame_transform::operator*=(const frame_transform &other)
{
	image_transform *= other.image_transform;
	audio_transform *= other.audio_transform;
	return *this;
}

frame_transform frame_transform::operator*(const frame_transform &other) const
{
	return frame_transform(*this) *= other;
}

frame_transform frame_transform::tween(double time, const frame_transform& source, const frame_transform& dest, double duration, const tweener& tween)
{
	frame_transform result;
	result.image_transform = image_transform::tween(time, source.image_transform, dest.image_transform, duration, tween);
	result.audio_transform = audio_transform::tween(time, source.audio_transform, dest.audio_transform, duration, tween);
	return result;
}

bool operator==(const frame_transform& lhs, const frame_transform& rhs)
{
	return	lhs.image_transform == rhs.image_transform && 
			lhs.audio_transform == rhs.audio_transform;
}

bool operator!=(const frame_transform& lhs, const frame_transform& rhs)
{
	return !(lhs == rhs);
}

}}