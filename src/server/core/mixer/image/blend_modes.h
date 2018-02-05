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

#pragma once

#include <string>

namespace caspar { namespace core {

enum class blend_mode
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

blend_mode   get_blend_mode(const std::wstring& str);
std::wstring get_blend_mode(blend_mode mode);

}} // namespace caspar::core