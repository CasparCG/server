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

blend_mode get_blend_mode(const std::wstring& str)
{
    if (boost::iequals(str, L"normal"))
        return blend_mode::normal;
    if (boost::iequals(str, L"lighten"))
        return blend_mode::lighten;
    if (boost::iequals(str, L"darken"))
        return blend_mode::darken;
    if (boost::iequals(str, L"multiply"))
        return blend_mode::multiply;
    if (boost::iequals(str, L"average"))
        return blend_mode::average;
    if (boost::iequals(str, L"add"))
        return blend_mode::add;
    if (boost::iequals(str, L"subtract"))
        return blend_mode::subtract;
    if (boost::iequals(str, L"difference"))
        return blend_mode::difference;
    if (boost::iequals(str, L"negation"))
        return blend_mode::negation;
    if (boost::iequals(str, L"exclusion"))
        return blend_mode::exclusion;
    if (boost::iequals(str, L"screen"))
        return blend_mode::screen;
    if (boost::iequals(str, L"overlay"))
        return blend_mode::overlay;
    if (boost::iequals(str, L"soft_light"))
        return blend_mode::soft_light;
    if (boost::iequals(str, L"hard_light"))
        return blend_mode::hard_light;
    if (boost::iequals(str, L"color_dodge"))
        return blend_mode::color_dodge;
    if (boost::iequals(str, L"color_burn"))
        return blend_mode::color_burn;
    if (boost::iequals(str, L"linear_dodge"))
        return blend_mode::linear_dodge;
    if (boost::iequals(str, L"linear_burn"))
        return blend_mode::linear_burn;
    if (boost::iequals(str, L"linear_light"))
        return blend_mode::linear_light;
    if (boost::iequals(str, L"vivid_light"))
        return blend_mode::vivid_light;
    if (boost::iequals(str, L"pin_light"))
        return blend_mode::pin_light;
    if (boost::iequals(str, L"hard_mix"))
        return blend_mode::hard_mix;
    if (boost::iequals(str, L"reflect"))
        return blend_mode::reflect;
    if (boost::iequals(str, L"glow"))
        return blend_mode::glow;
    if (boost::iequals(str, L"phoenix"))
        return blend_mode::phoenix;
    if (boost::iequals(str, L"contrast"))
        return blend_mode::contrast;
    if (boost::iequals(str, L"saturation"))
        return blend_mode::saturation;
    if (boost::iequals(str, L"color"))
        return blend_mode::color;
    if (boost::iequals(str, L"luminosity"))
        return blend_mode::luminosity;

    return blend_mode::normal;
}

std::wstring get_blend_mode(blend_mode mode)
{
    switch (mode) {
        case blend_mode::normal:
            return L"normal";
        case blend_mode::lighten:
            return L"lighten";
        case blend_mode::darken:
            return L"darken";
        case blend_mode::multiply:
            return L"multiply";
        case blend_mode::average:
            return L"average";
        case blend_mode::add:
            return L"add";
        case blend_mode::subtract:
            return L"subtract";
        case blend_mode::difference:
            return L"difference";
        case blend_mode::negation:
            return L"negation";
        case blend_mode::exclusion:
            return L"exclusion";
        case blend_mode::screen:
            return L"screen";
        case blend_mode::overlay:
            return L"overlay";
        case blend_mode::soft_light:
            return L"soft_light";
        case blend_mode::hard_light:
            return L"hard_light";
        case blend_mode::color_dodge:
            return L"color_dodge";
        case blend_mode::color_burn:
            return L"color_burn";
        case blend_mode::linear_dodge:
            return L"linear_dodge";
        case blend_mode::linear_burn:
            return L"linear_burn";
        case blend_mode::linear_light:
            return L"linear_light";
        case blend_mode::vivid_light:
            return L"vivid_light";
        case blend_mode::pin_light:
            return L"pin_light";
        case blend_mode::hard_mix:
            return L"hard_mix";
        case blend_mode::reflect:
            return L"reflect";
        case blend_mode::glow:
            return L"glow";
        case blend_mode::phoenix:
            return L"phoenix";
        case blend_mode::contrast:
            return L"contrast";
        case blend_mode::saturation:
            return L"saturation";
        case blend_mode::color:
            return L"color";
        case blend_mode::luminosity:
            return L"luminosity";
        default:
            return L"normal";
    }
}

}} // namespace caspar::core
