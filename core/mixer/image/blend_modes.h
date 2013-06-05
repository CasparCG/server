/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#pragma once

namespace caspar { namespace core {
		
struct chroma
{
    enum type
    {
        none     = 0x000000,
        red      = 0xff0000,
        yellow   = 0xffff00,
        green    = 0x00ff00,
        torquise = 0x00ffff,
        blue     = 0x0000ff,
        magenta  = 0xff00ff
    };

    type	key;
    float	threshold;
    float	softness;
	float	spill;
    float	blur;
    bool	show_mask;

    chroma(type m = none)
		: key(m)
		, threshold(0.0)
		, softness(0.0)
		, spill(0.0)
		, blur(0.0)
		, show_mask(false)
	{
	}
};

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
		mix,
		blend_mode_count 
	};

	type	mode;
	chroma	chroma;

	blend_mode(type t = normal) : mode(t) {}
};

blend_mode::type get_blend_mode(const std::wstring& str);
std::wstring get_blend_mode(blend_mode::type mode);

chroma::type get_chroma_mode(const std::wstring& str);
std::wstring get_chroma_mode(chroma::type type);

}}
