/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../../stdafx.h"

#include "image_transform.h"

#include <common/utility/assert.h>

namespace caspar { namespace core {
		
image_transform::image_transform() 
	: opacity_(1.0)
	, gain_(1.0)
	, mode_(video_mode::invalid)
	, is_key_(false)
	, deinterlace_(false)
	, blend_mode_(image_transform::normal)
{
	std::fill(fill_translation_.begin(), fill_translation_.end(), 0.0);
	std::fill(fill_scale_.begin(), fill_scale_.end(), 1.0);
	std::fill(clip_translation_.begin(), clip_translation_.end(), 0.0);
	std::fill(clip_scale_.begin(), clip_scale_.end(), 1.0);
}

void image_transform::set_opacity(double value)
{
	opacity_ = std::max(value, 0.0);
}

double image_transform::get_opacity() const
{
	return opacity_;
}

void image_transform::set_gain(double value)
{
	gain_ = std::max(0.0, value);
}

double image_transform::get_gain() const
{
	return gain_;
}

void image_transform::set_fill_translation(double x, double y)
{
	fill_translation_[0] = x;
	fill_translation_[1] = y;
}

void image_transform::set_fill_scale(double x, double y)
{
	fill_scale_[0] = x;
	fill_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_fill_translation() const
{
	return fill_translation_;
}

std::array<double, 2> image_transform::get_fill_scale() const
{
	return fill_scale_;
}

void image_transform::set_clip_translation(double x, double y)
{
	clip_translation_[0] = x;
	clip_translation_[1] = y;
}

void image_transform::set_clip_scale(double x, double y)
{
	clip_scale_[0] = x;
	clip_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_clip_translation() const
{
	return clip_translation_;
}

std::array<double, 2> image_transform::get_clip_scale() const
{
	return clip_scale_;
}

void image_transform::set_mode(video_mode::type mode)
{
	mode_ = mode;
}

video_mode::type image_transform::get_mode() const
{
	return mode_;
}

void image_transform::set_deinterlace(bool value)
{
	deinterlace_ = value;
}

bool image_transform::get_deinterlace() const
{
	return deinterlace_;
}

void image_transform::set_blend_mode(image_transform::blend_mode value)
{
	blend_mode_ = value;
}

image_transform::blend_mode image_transform::get_blend_mode() const
{
	return blend_mode_;
}

image_transform& image_transform::operator*=(const image_transform &other)
{
	opacity_				*= other.opacity_;
	
	if(other.mode_ != video_mode::invalid)
		mode_ = other.mode_;

	blend_mode_				 = other.blend_mode_;
	gain_					*= other.gain_;
	deinterlace_			|= other.deinterlace_;
	is_key_					|= other.is_key_;
	fill_translation_[0]	+= other.fill_translation_[0]*fill_scale_[0];
	fill_translation_[1]	+= other.fill_translation_[1]*fill_scale_[1];
	fill_scale_[0]			*= other.fill_scale_[0];
	fill_scale_[1]			*= other.fill_scale_[1];
	clip_translation_[0]	+= other.clip_translation_[0]*clip_scale_[0];
	clip_translation_[1]	+= other.clip_translation_[1]*clip_scale_[1];
	clip_scale_[0]			*= other.clip_scale_[0];
	clip_scale_[1]			*= other.clip_scale_[1];
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

void image_transform::set_is_key(bool value){is_key_ = value;}
bool image_transform::get_is_key() const{return is_key_;}

image_transform tween(double time, const image_transform& source, const image_transform& dest, double duration, const tweener_t& tweener)
{	
	auto do_tween = [](double time, double source, double dest, double duration, const tweener_t& tweener)
	{
		return tweener(time, source, dest-source, duration);
	};

	CASPAR_ASSERT(source.get_mode() == dest.get_mode() || source.get_mode() == video_mode::invalid || dest.get_mode() == video_mode::invalid);

	image_transform result;	
	result.set_mode				(dest.get_mode() != video_mode::invalid ? dest.get_mode() : source.get_mode());
	result.set_blend_mode		(dest.get_blend_mode());
	result.set_is_key			(source.get_is_key() | dest.get_is_key());
	result.set_deinterlace		(source.get_deinterlace() | dest.get_deinterlace());
	result.set_gain				(do_tween(time, source.get_gain(), dest.get_gain(), duration, tweener));
	result.set_opacity			(do_tween(time, source.get_opacity(), dest.get_opacity(), duration, tweener));
	result.set_fill_translation	(do_tween(time, source.get_fill_translation()[0], dest.get_fill_translation()[0], duration, tweener), do_tween(time, source.get_fill_translation()[1], dest.get_fill_translation()[1], duration, tweener));
	result.set_fill_scale		(do_tween(time, source.get_fill_scale()[0], dest.get_fill_scale()[0], duration, tweener), do_tween(time, source.get_fill_scale()[1], dest.get_fill_scale()[1], duration, tweener));
	result.set_clip_translation	(do_tween(time, source.get_clip_translation()[0], dest.get_clip_translation()[0], duration, tweener), do_tween(time, source.get_clip_translation()[1], dest.get_clip_translation()[1], duration, tweener));
	result.set_clip_scale		(do_tween(time, source.get_clip_scale()[0], dest.get_clip_scale()[0], duration, tweener), do_tween(time, source.get_clip_scale()[1], dest.get_clip_scale()[1], duration, tweener));
	
	return result;
}

}}