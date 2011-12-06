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

#include "../../StdAfx.h"

#include "blend_modes.h"

#include <boost/algorithm/string.hpp>

namespace caspar { namespace core {
		
blend_mode::type get_blend_mode(const std::string& str)
{
	if(iequals(str, "normal"))
		return blend_mode::normal;
	else if(iequals(str, "lighten"))
		return blend_mode::lighten;
	else if(iequals(str, "darken"))
		return blend_mode::darken;
	else if(iequals(str, "multiply"))
		return blend_mode::multiply;
	else if(iequals(str, "average"))
		return blend_mode::average;
	else if(iequals(str, "add"))
		return blend_mode::add;
	else if(iequals(str, "subtract"))
		return blend_mode::subtract;
	else if(iequals(str, "difference"))
		return blend_mode::difference;
	else if(iequals(str, "negation"))
		return blend_mode::negation;
	else if(iequals(str, "exclusion"))
		return blend_mode::exclusion;
	else if(iequals(str, "screen"))
		return blend_mode::screen;
	else if(iequals(str, "overlay"))
		return blend_mode::overlay;
	else if(iequals(str, "soft_light"))
		return blend_mode::soft_light;
	else if(iequals(str, "hard_light"))
		return blend_mode::hard_light;
	else if(iequals(str, "color_dodge"))
		return blend_mode::color_dodge;
	else if(iequals(str, "color_burn"))
		return blend_mode::color_burn;
	else if(iequals(str, "linear_dodge"))
		return blend_mode::linear_dodge;
	else if(iequals(str, "linear_burn"))
		return blend_mode::linear_burn;
	else if(iequals(str, "linear_light"))
		return blend_mode::linear_light;
	else if(iequals(str, "vivid_light"))
		return blend_mode::vivid_light;
	else if(iequals(str, "pin_light"))
		return blend_mode::pin_light;
	else if(iequals(str, "hard_mix"))
		return blend_mode::hard_mix;
	else if(iequals(str, "reflect"))
		return blend_mode::reflect;
	else if(iequals(str, "glow"))
		return blend_mode::glow;
	else if(iequals(str, "phoenix"))
		return blend_mode::phoenix;
	else if(iequals(str, "contrast"))
		return blend_mode::contrast;
	else if(iequals(str, "saturation"))
		return blend_mode::saturation;
	else if(iequals(str, "color"))
		return blend_mode::color;
	else if(iequals(str, "luminosity"))
		return blend_mode::luminosity;
		
	return blend_mode::normal;
}

}}