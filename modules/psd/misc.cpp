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

#include <vector>
#include "misc.h"

namespace caspar { namespace psd {

blend_mode int_to_blend_mode(unsigned long x)
{
	blend_mode retVal = InvalidBlendMode;
	switch(x)
	{
		case Normal: retVal = Normal; break;
		case Darken: retVal = Darken; break;
		case Lighten: retVal = Lighten; break;
		case Hue: retVal = Hue; break;
		case Saturation: retVal = Saturation; break;
		case Color: retVal = Color; break;
		case Luminosity: retVal = Luminosity; break;
		case Multiply: retVal = Multiply; break;
		case Screen: retVal = Screen; break;
		case Dissolve: retVal = Dissolve; break;
		case Overlay: retVal = Overlay; break;
		case HardLight: retVal = HardLight; break;
		case SoftLight: retVal = SoftLight; break;
		case Difference: retVal = Difference; break;
		case Exclusion: retVal = Exclusion; break;
		case ColorDodge: retVal = ColorDodge; break;
		case ColorBurn: retVal = ColorBurn; break;
	}

	return retVal;
}

std::wstring blend_mode_to_string(blend_mode b)
{
	std::wstring retVal = L"Invalid";
	switch(b)
	{
		case Normal: retVal = L"Normal"; break;
		case Darken: retVal = L"Darken"; break;
		case Lighten: retVal = L"Lighten"; break;
		case Hue: retVal = L"Hue"; break;
		case Saturation: retVal = L"Saturation"; break;
		case Color: retVal = L"Color"; break;
		case Luminosity: retVal = L"Luminosity"; break;
		case Multiply: retVal = L"Multiply"; break;
		case Screen: retVal = L"Screen"; break;
		case Dissolve: retVal = L"Dissolve"; break;
		case Overlay: retVal = L"Overlay"; break;
		case HardLight: retVal = L"HardLight"; break;
		case SoftLight: retVal = L"SoftLight"; break;
		case Difference: retVal = L"Difference"; break;
		case Exclusion: retVal = L"Exclusion"; break;
		case ColorDodge: retVal = L"ColorDodge"; break;
		case ColorBurn: retVal = L"ColorBurn"; break;
	}

	return retVal;
}

color_mode int_to_color_mode(unsigned short x)
{
	color_mode retVal = InvalidColorMode;
	switch(x)
	{
		case Bitmap: retVal = Bitmap; break;
		case Grayscale:	retVal = Grayscale;	break;
		case Indexed: retVal = Indexed;	break;
		case RGB: retVal = RGB;	break;
		case CMYK: retVal = CMYK; break;
		case Multichannel: retVal = Multichannel; break;
		case Duotone: retVal = Duotone; break;
		case Lab: retVal = Lab;	break;
	};

	return retVal;
}
std::wstring color_mode_to_string(color_mode c)
{
	std::wstring retVal = L"Invalid";
	switch(c)
	{
		case Bitmap: retVal = L"Bitmap"; break;
		case Grayscale:	retVal = L"Grayscale";	break;
		case Indexed: retVal = L"Indexed";	break;
		case RGB: retVal = L"RGB";	break;
		case CMYK: retVal = L"CMYK"; break;
		case Multichannel: retVal = L"Multichannel"; break;
		case Duotone: retVal = L"Duotone"; break;
		case Lab: retVal = L"Lab";	break;
	};

	return retVal;
}

}	//namespace psd
}	//namespace caspar