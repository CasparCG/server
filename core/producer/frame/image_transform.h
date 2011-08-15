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
#pragma once

#include <common/utility/tweener.h>
#include <core/video_format.h>

#include <array>
#include <type_traits>

namespace caspar { namespace core {

struct pixel_format_desc;
	
class image_transform 
{
public:
	
	struct blend_mode
	{
		enum type 
		{
			normal = 0,
			lighten,
			darken,
			multiply,
			average,
			add,
			subtract,
			difference,
			negation,
			exclusion,
			screen,
			overlay,
			soft_light,
			hard_light,
			color_dodge,
			color_burn,
			linear_dodge,
			linear_burn,
			linear_light,
			vivid_light,
			pin_light,
			hard_mix,
			reflect,
			glow,
			phoenix,
			contrast,
			saturation,
			color,
			luminosity,
			blend_mode_count 
		};
	};
	
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

	image_transform();

	void set_opacity(double value);
	double get_opacity() const;

	void set_gain(double value);
	double get_gain() const;

	void set_brightness(double value);
	double get_brightness() const;

	void set_contrast(double value);
	double get_contrast() const;

	void set_saturation(double value);
	double get_saturation() const;
	
	void set_levels(const levels& value);
	levels get_levels() const;
	
	void set_fill_translation(double x, double y);
	std::array<double, 2> get_fill_translation() const;

	void set_fill_scale(double x, double y);
	std::array<double, 2> get_fill_scale() const;
	
	void set_clip_translation(double x, double y);
	std::array<double, 2> get_clip_translation() const;

	void set_clip_scale(double x, double y);
	std::array<double, 2> get_clip_scale() const;
	
	image_transform& operator*=(const image_transform &other);
	const image_transform operator*(const image_transform &other) const;

	void set_is_key(bool value);
	bool get_is_key() const;

	void set_is_atomic(bool value);
	bool get_is_atomic() const;

	void set_blend_mode(blend_mode::type value);
	blend_mode::type get_blend_mode() const;
	
private:
	double opacity_;
	double gain_;
	double contrast_;
	double brightness_;
	double saturation_;
	double desaturation_;
	levels levels_;
	std::array<double, 2> fill_translation_; 
	std::array<double, 2> fill_scale_; 
	std::array<double, 2> clip_translation_; 
	std::array<double, 2> clip_scale_; 
	video_mode::type mode_;
	bool is_key_;
	bool is_atomic_;
	blend_mode::type blend_mode_;
};

image_transform::blend_mode::type get_blend_mode(const std::wstring& str);

image_transform tween(double time, const image_transform& source, const image_transform& dest, double duration, const tweener_t& tweener);

bool operator<(const image_transform& lhs, const image_transform& rhs);
bool operator==(const image_transform& lhs, const image_transform& rhs);
bool operator!=(const image_transform& lhs, const image_transform& rhs);

}}