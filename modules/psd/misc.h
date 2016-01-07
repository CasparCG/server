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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#pragma once

#include <common/except.h>
#include <common/enum_class.h>
#include <core/mixer/image/blend_modes.h>

#include <string>
#include <cstdint>
#include <vector>

namespace caspar { namespace psd {
	
template<typename T>
struct point
{
	point() : x(0), y(0) {}
	point(T x1, T y1) : x(x1), y(y1) {}
	T x;
	T y;

	void clear() { x = 0; y = 0; }
	
	bool operator==(const point& rhs) {
		return x == rhs.x && y == rhs.y;
	}
};

template<typename T>
struct color
{
	T red		= 0;
	T green		= 0;
	T blue		= 0;
	T alpha		= 0;

	std::uint32_t to_uint32()
	{
		return (alpha << 24) + (red << 16) + (green << 8) + blue;
	}
};

template<typename T>
struct size
{
	size() : width(0), height(0) {}
	size(T w, T h) : width(w), height(h) {}
	T width;
	T height;

	void clear() { width = 0; height = 0; }
};

template<typename T>
struct rect
{
	point<T>		location;
	psd::size<T>	size;

	bool empty() const { return size.width == 0 || size.height == 0; }
	void clear() { location.clear(); size.clear(); }
};

struct psd_file_format_exception : virtual caspar_exception {};

enum class channel_type
{
	total_user_mask = -3,
	user_mask = -2,
	transparency = -1,
	color_red = 0,
	color_green = 1,
	color_blue = 2
};

enum class layer_type
{
	content = 0,
	group,
	timeline_group,
	group_delimiter
};
layer_type int_to_layer_type(std::uint32_t x, std::uint32_t y);
std::wstring layer_type_to_string(layer_type b);

caspar::core::blend_mode int_to_blend_mode(std::uint32_t x);

enum class color_mode
{
	InvalidColorMode = -1,
	Bitmap = 0,
	Grayscale = 1,
	Indexed = 2,
	RGB = 3,
	CMYK = 4,
	Multichannel = 7,
	Duotone = 8,
	Lab = 9
};

color_mode int_to_color_mode(std::uint16_t x);
std::wstring color_mode_to_string(color_mode c);


enum class layer_tag : int {
	none = 0,
	placeholder = 1,
	explicit_dynamic = 2,
	moveable = 4,
	resizable = 8,
	rasterized = 16,
	cornerpin = 32//,
	//all = 63
};
ENUM_ENABLE_BITWISE(layer_tag);

/*inline layer_tag operator ~ (layer_tag rhs)
{
	return (layer_tag)(static_cast<int>(layer_tag::all) ^ static_cast<int>(rhs));
}*/

layer_tag string_to_layer_tags(const std::wstring& str);

}	//namespace psd
}	//namespace caspar

