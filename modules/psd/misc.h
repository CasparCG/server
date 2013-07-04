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
struct rect
{
	T top;
	T right;
	T bottom;
	T left;
	T width() const { return right-left; }
	T height() const { return bottom-top; }
};

class PSDFileFormatException : public std::exception
{
public:
	virtual ~PSDFileFormatException()
	{}
	virtual const char *what() const
	{
		return "Unknown fileformat error";
	}
};

enum ChannelType
{
	TotalUserMask = -3,
	UserMask = -2,
	Transparency = -1,
	ColorRed = 0,
	ColorGreen = 1,
	ColorBlue = 2
};

enum BlendMode
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

BlendMode IntToBlendMode(unsigned long x);
std::wstring BlendModeToString(BlendMode b);

enum color_mode
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