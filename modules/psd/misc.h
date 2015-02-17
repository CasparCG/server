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

#ifndef _PSDMISC_H__
#define _PSDMISC_H__

#include <string>

namespace caspar { namespace psd {
	
template<typename T>
struct point
{
	point() : x(0), y(0) {}
	point(T x1, T y1) : x(x1), y(y1) {}
	T x;
	T y;
};

template<typename T>
struct color
{
	color() : red(0), green(0), blue(0), alpha(0)
	{}

	T red;
	T green;
	T blue;
	T alpha;

	unsigned long to_uint32()
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
};

template<typename T>
struct rect
{
	point<T>	location;
	size<T>		size;

	bool empty() const { return size.width == 0 || size.height == 0; }
};



class PSDFileFormatException : public std::exception
{
public:
	PSDFileFormatException() : std::exception()
	{}
	explicit PSDFileFormatException(const char* msg) : std::exception(msg)
	{}

	virtual ~PSDFileFormatException()
	{}
	virtual const char *what() const
	{
		return "Unknown fileformat error";
	}
};

enum class channel_type
{
	total_user_mask = -3,
	user_mask = -2,
	transparency = -1,
	color_red = 0,
	color_green = 1,
	color_blue = 2
};

enum class blend_mode
{
	InvalidBlendMode = -1,
	Normal = 'norm',
	Darken = 'dark',
	Lighten = 'lite',
	Hue = 'hue ',
	Saturation = 'sat ',
	Color = 'colr',
	Luminosity = 'lum ',
	Multiply = 'mul ',
	Screen = 'scrn',
	Dissolve = 'diss',
	Overlay = 'over',
	HardLight = 'hLit',
	SoftLight = 'sLit',
	Difference = 'diff',
	Exclusion = 'smud',
	ColorDodge = 'div ',
	ColorBurn = 'idiv'
};

blend_mode int_to_blend_mode(unsigned long x);
std::wstring blend_mode_to_string(blend_mode b);

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

color_mode int_to_color_mode(unsigned short x);
std::wstring color_mode_to_string(color_mode c);

}	//namespace psd
}	//namespace caspar

#endif