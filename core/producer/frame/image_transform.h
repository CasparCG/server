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
	image_transform();

	void set_opacity(double value);
	double get_opacity() const;

	void set_gain(double value);
	double get_gain() const;
	
	void set_fill_translation(double x, double y);
	std::array<double, 2> get_fill_translation() const;

	void set_fill_scale(double x, double y);
	std::array<double, 2> get_fill_scale() const;
	
	void set_clip_translation(double x, double y);
	std::array<double, 2> get_clip_translation() const;

	void set_clip_scale(double x, double y);
	std::array<double, 2> get_clip_scale() const;

	void set_mode(video_mode::type mode);
	video_mode::type get_mode() const;

	image_transform& operator*=(const image_transform &other);
	const image_transform operator*(const image_transform &other) const;

	void set_is_key(bool value);
	bool get_is_key() const;

	void set_deinterlace(bool value);
	bool get_deinterlace() const;

private:
	double opacity_;
	double gain_;
	std::array<double, 2> fill_translation_; 
	std::array<double, 2> fill_scale_; 
	std::array<double, 2> clip_translation_; 
	std::array<double, 2> clip_scale_; 
	video_mode::type mode_;
	bool is_key_;
	bool deinterlace_;
};

image_transform tween(double time, const image_transform& source, const image_transform& dest, double duration, const tweener_t& tweener);

inline bool operator==(const image_transform& lhs, const image_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(image_transform)) == 0;
}

inline bool operator!=(const image_transform& lhs, const image_transform& rhs)
{
	return !(lhs == rhs);
}

}}