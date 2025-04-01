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

#include <vector>

#include <common/bit_depth.h>

namespace caspar { namespace core {

enum class pixel_format
{
    gray = 0,
    bgra,
    rgba,
    argb,
    abgr,
    ycbcr,
    ycbcra,
    luma,
    bgr,
    rgb,
    uyvy,
    gbrp, // planar
    gbrap, // planar
    count,
    invalid,
};

enum class color_space
{
    bt601,
    bt709,
    bt2020,
};

struct pixel_format_desc final
{
    struct plane
    {
        int               linesize = 0;
        int               width    = 0;
        int               height   = 0;
        int               size     = 0;
        int               stride   = 0;
        common::bit_depth depth    = common::bit_depth::bit8;

        plane() = default;

        plane(int width, int height, int stride, common::bit_depth depth = common::bit_depth::bit8)
            : linesize(width * stride * (depth == common::bit_depth::bit8 ? 1 : 2))
            , width(width)
            , height(height)
            , size(width * height * stride * (depth == common::bit_depth::bit8 ? 1 : 2))
            , stride(stride)
            , depth(depth)
        {
        }
    };

    pixel_format_desc() = default;

    explicit pixel_format_desc(pixel_format format, core::color_space color_space = core::color_space::bt709)
        : format(format)
        , color_space(color_space)
    {
    }

    pixel_format       format            = pixel_format::invalid;
    bool               is_straight_alpha = false;
    std::vector<plane> planes;
    core::color_space  color_space = core::color_space::bt709;
};

}} // namespace caspar::core
