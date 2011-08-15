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
// TODO: Move layer specific stuff out of frame related classes.
#include "../../stdafx.h"

#include "image_transform.h"

#include <common/utility/assert.h>

#include <boost/algorithm/string.hpp>

namespace caspar { namespace core {
		
image_transform::image_transform() 
	: opacity_(1.0)
	, gain_(1.0)
	, brightness_(1.0)
	, contrast_(1.0)
	, saturation_(1.0)
	, is_key_(false)
	, blend_mode_(image_transform::blend_mode::normal)
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

void image_transform::set_brightness(double value)
{
	brightness_ = std::max(0.0, value);
}

double image_transform::get_brightness() const
{
	return brightness_;
}

void image_transform::set_contrast(double value)
{
	contrast_ = std::max(0.0, value);
}

double image_transform::get_contrast() const
{
	return contrast_;
}

void image_transform::set_saturation(double value)
{
	saturation_ = std::max(0.0, value);
}

double image_transform::get_saturation() const
{
	return saturation_;
}

void image_transform::set_levels(const image_transform::levels& value)
{
	levels_ = value;
}

image_transform::levels image_transform::get_levels() const
{
	return levels_;
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

void image_transform::set_blend_mode(image_transform::blend_mode::type value)
{
	blend_mode_ = value;
}

image_transform::blend_mode::type image_transform::get_blend_mode() const
{
	return blend_mode_;
}

image_transform& image_transform::operator*=(const image_transform &other)
{
	opacity_				*= other.opacity_;	
	blend_mode_				 = std::max(blend_mode_, other.blend_mode_);
	gain_					*= other.gain_;
	brightness_				*= other.brightness_;
	contrast_				*= other.contrast_;
	saturation_				*= other.saturation_;

	levels_.min_input		= std::max(levels_.min_input, other.levels_.min_input);
	levels_.max_input		= std::min(levels_.max_input, other.levels_.max_input);
	
	levels_.min_output		= std::max(levels_.min_output, other.levels_.min_output);
	levels_.max_output		= std::min(levels_.max_output, other.levels_.max_output);

	levels_.gamma			*= other.levels_.gamma;

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
	
	image_transform result;	
	result.set_blend_mode		(std::max(source.get_blend_mode(), dest.get_blend_mode()));
	result.set_is_key			(source.get_is_key() | dest.get_is_key());
	result.set_gain				(do_tween(time, source.get_gain(), dest.get_gain(), duration, tweener));
	result.set_brightness		(do_tween(time, source.get_brightness(), dest.get_brightness(), duration, tweener));
	result.set_contrast			(do_tween(time, source.get_contrast(), dest.get_contrast(), duration, tweener));
	result.set_saturation		(do_tween(time, source.get_saturation(), dest.get_saturation(), duration, tweener));
	result.set_opacity			(do_tween(time, source.get_opacity(), dest.get_opacity(), duration, tweener));
	result.set_fill_translation	(do_tween(time, source.get_fill_translation()[0], dest.get_fill_translation()[0], duration, tweener), do_tween(time, source.get_fill_translation()[1], dest.get_fill_translation()[1], duration, tweener));
	result.set_fill_scale		(do_tween(time, source.get_fill_scale()[0], dest.get_fill_scale()[0], duration, tweener), do_tween(time, source.get_fill_scale()[1], dest.get_fill_scale()[1], duration, tweener));
	result.set_clip_translation	(do_tween(time, source.get_clip_translation()[0], dest.get_clip_translation()[0], duration, tweener), do_tween(time, source.get_clip_translation()[1], dest.get_clip_translation()[1], duration, tweener));
	result.set_clip_scale		(do_tween(time, source.get_clip_scale()[0], dest.get_clip_scale()[0], duration, tweener), do_tween(time, source.get_clip_scale()[1], dest.get_clip_scale()[1], duration, tweener));
	
	auto s_levels = source.get_levels();
	auto d_levels = dest.get_levels();

	d_levels.max_input = do_tween(time, s_levels.max_input, d_levels.max_input, duration, tweener);
	d_levels.min_input = do_tween(time, s_levels.min_input, d_levels.min_input, duration, tweener);
	
	d_levels.max_output = do_tween(time, s_levels.max_output, d_levels.max_output, duration, tweener);
	d_levels.min_output = do_tween(time, s_levels.min_output, d_levels.min_output, duration, tweener);

	d_levels.gamma = do_tween(time, s_levels.gamma, d_levels.gamma, duration, tweener);

	result.set_levels(d_levels);

	return result;
}

image_transform::blend_mode::type get_blend_mode(const std::wstring& str)
{
	if(boost::iequals(str, L"normal"))
		return image_transform::blend_mode::normal;
	else if(boost::iequals(str, L"lighten"))
		return image_transform::blend_mode::lighten;
	else if(boost::iequals(str, L"darken"))
		return image_transform::blend_mode::darken;
	else if(boost::iequals(str, L"multiply"))
		return image_transform::blend_mode::multiply;
	else if(boost::iequals(str, L"average"))
		return image_transform::blend_mode::average;
	else if(boost::iequals(str, L"add"))
		return image_transform::blend_mode::add;
	else if(boost::iequals(str, L"subtract"))
		return image_transform::blend_mode::subtract;
	else if(boost::iequals(str, L"difference"))
		return image_transform::blend_mode::difference;
	else if(boost::iequals(str, L"negation"))
		return image_transform::blend_mode::negation;
	else if(boost::iequals(str, L"exclusion"))
		return image_transform::blend_mode::exclusion;
	else if(boost::iequals(str, L"screen"))
		return image_transform::blend_mode::screen;
	else if(boost::iequals(str, L"overlay"))
		return image_transform::blend_mode::overlay;
	else if(boost::iequals(str, L"soft_light"))
		return image_transform::blend_mode::soft_light;
	else if(boost::iequals(str, L"hard_light"))
		return image_transform::blend_mode::hard_light;
	else if(boost::iequals(str, L"color_dodge"))
		return image_transform::blend_mode::color_dodge;
	else if(boost::iequals(str, L"color_burn"))
		return image_transform::blend_mode::color_burn;
	else if(boost::iequals(str, L"linear_dodge"))
		return image_transform::blend_mode::linear_dodge;
	else if(boost::iequals(str, L"linear_burn"))
		return image_transform::blend_mode::linear_burn;
	else if(boost::iequals(str, L"linear_light"))
		return image_transform::blend_mode::linear_light;
	else if(boost::iequals(str, L"vivid_light"))
		return image_transform::blend_mode::vivid_light;
	else if(boost::iequals(str, L"pin_light"))
		return image_transform::blend_mode::pin_light;
	else if(boost::iequals(str, L"hard_mix"))
		return image_transform::blend_mode::hard_mix;
	else if(boost::iequals(str, L"reflect"))
		return image_transform::blend_mode::reflect;
	else if(boost::iequals(str, L"glow"))
		return image_transform::blend_mode::glow;
	else if(boost::iequals(str, L"phoenix"))
		return image_transform::blend_mode::phoenix;
	else if(boost::iequals(str, L"contrast"))
		return image_transform::blend_mode::contrast;
	else if(boost::iequals(str, L"saturation"))
		return image_transform::blend_mode::saturation;
	else if(boost::iequals(str, L"color"))
		return image_transform::blend_mode::color;
	else if(boost::iequals(str, L"luminosity"))
		return image_transform::blend_mode::luminosity;
		
	return image_transform::blend_mode::normal;
}

bool operator<(const image_transform& lhs, const image_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(image_transform)) < 0;
}

bool operator==(const image_transform& lhs, const image_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(image_transform)) == 0;
}

bool operator!=(const image_transform& lhs, const image_transform& rhs)
{
	return !(lhs == rhs);
}

}}