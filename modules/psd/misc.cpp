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

#include <boost/algorithm/string.hpp>
#include <vector>

#include "misc.h"

namespace caspar { namespace psd {

layer_type int_to_layer_type(std::uint32_t x, std::uint32_t y)
{
	if (x == 1 || x == 2)
		return (y == 1) ? layer_type::timeline_group : layer_type::group;
	else if (x == 3)
		return layer_type::group_delimiter;

	return layer_type::content;
}

std::wstring layer_type_to_string(layer_type b)
{
	switch (b)
	{
	case layer_type::content: return L"Content";
	case layer_type::group: return L"Group";
	case layer_type::timeline_group: return L"TimelineGroup";
	case layer_type::group_delimiter: return L"GroupDelimiter";
	default: return L"Invalid";
	}
}


blend_mode int_to_blend_mode(std::uint32_t x)
{
	blend_mode mode = static_cast<blend_mode>(x);

	switch (mode)
	{
		case blend_mode::Normal:
		case blend_mode::Darken:
		case blend_mode::Lighten:
		case blend_mode::Hue:
		case blend_mode::Saturation:
		case blend_mode::Color:
		case blend_mode::Luminosity:
		case blend_mode::Multiply:
		case blend_mode::Screen:
		case blend_mode::Dissolve:
		case blend_mode::Overlay:
		case blend_mode::HardLight:
		case blend_mode::SoftLight:
		case blend_mode::Difference:
		case blend_mode::Exclusion:
		case blend_mode::ColorDodge:
		case blend_mode::ColorBurn:
			return mode;
		default:
			return blend_mode::InvalidBlendMode;
	}
}

std::wstring blend_mode_to_string(blend_mode b)
{
	switch(b)
	{
		case blend_mode::Normal: return L"Normal";
		case blend_mode::Darken: return L"Darken";
		case blend_mode::Lighten: return L"Lighten";
		case blend_mode::Hue: return L"Hue";
		case blend_mode::Saturation: return L"Saturation";
		case blend_mode::Color: return L"Color";
		case blend_mode::Luminosity: return L"Luminosity";
		case blend_mode::Multiply: return L"Multiply";
		case blend_mode::Screen: return L"Screen";
		case blend_mode::Dissolve: return L"Dissolve";
		case blend_mode::Overlay: return L"Overlay";
		case blend_mode::HardLight: return L"HardLight";
		case blend_mode::SoftLight: return L"SoftLight";
		case blend_mode::Difference: return L"Difference";
		case blend_mode::Exclusion: return L"Exclusion";
		case blend_mode::ColorDodge: return L"ColorDodge";
		case blend_mode::ColorBurn: return L"ColorBurn";
		default: return L"Invalid";
	}
}

color_mode int_to_color_mode(std::uint16_t x)
{
	color_mode mode = static_cast<color_mode>(x);

	switch(mode)
	{
		case color_mode::Bitmap:
		case color_mode::Grayscale:
		case color_mode::Indexed:
		case color_mode::RGB:
		case color_mode::CMYK:
		case color_mode::Multichannel:
		case color_mode::Duotone:
		case color_mode::Lab:
			return mode;
		default:
			return color_mode::InvalidColorMode;
	};
}

std::wstring color_mode_to_string(color_mode c)
{
	switch(c)
	{
		case color_mode::Bitmap: return L"Bitmap";
		case color_mode::Grayscale:	return L"Grayscale";
		case color_mode::Indexed: return L"Indexed";
		case color_mode::RGB: return L"RGB";
		case color_mode::CMYK: return L"CMYK";
		case color_mode::Multichannel: return L"Multichannel";
		case color_mode::Duotone: return L"Duotone";
		case color_mode::Lab: return L"Lab";
		default: return L"Invalid";
	};
}

layer_tag string_to_layer_tags(const std::wstring& str) {
	std::vector<std::wstring> flags;
	boost::split(flags, str, boost::is_any_of(L", "), boost::token_compress_on);

	layer_tag result = layer_tag::none;
	for (auto& flag : flags) {
		if (boost::algorithm::iequals(flag, "producer"))
			result = result | layer_tag::placeholder;
		else if (boost::algorithm::iequals(flag, "dynamic")) {
			result = result | layer_tag::explicit_dynamic;
			result = result & (~layer_tag::rasterized);
		}
		else if (boost::algorithm::iequals(flag, "static")) {
			result = result | layer_tag::rasterized;
			result = result & (~layer_tag::explicit_dynamic);
		}
		else if (boost::algorithm::iequals(flag, "movable")) {
			result = result | layer_tag::moveable;
			result = result & (~layer_tag::resizable);
			result = result & (~layer_tag::explicit_dynamic);
		}
		else if (boost::algorithm::iequals(flag, "resizable")) {
			result = result | layer_tag::resizable;
			result = result & (~layer_tag::moveable);
			result = result & (~layer_tag::explicit_dynamic);
		}
	}

	return result;
}


}	//namespace psd
}	//namespace caspar
