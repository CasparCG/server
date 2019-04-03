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
    count,
    invalid,
};

struct pixel_format_desc final
{
    struct plane
    {
        int linesize = 0;
        int width    = 0;
        int height   = 0;
        int size     = 0;
        int stride   = 0;

        plane() = default;

        plane(int width, int height, int stride)
            : linesize(width * stride)
            , width(width)
            , height(height)
            , size(width * height * stride)
            , stride(stride)
        {
        }
    };

    pixel_format_desc() = default;

    pixel_format_desc(pixel_format format)
        : format(format)
    {
    }

    pixel_format       format = pixel_format::invalid;
    std::vector<plane> planes;
};

}} // namespace caspar::core
