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
 * Author: Julian Waller, julian@superfly.tv
 */

#pragma once

#include <vector>

#pragma warning(push)
#pragma warning(disable : 4458)
#include <algorithm>
#include <include/cef_render_handler.h>
#pragma warning(pop)

namespace caspar::html {

struct Rectangle
{
    Rectangle(int l_x, int l_y, int r_x, int r_y)
        : l_x(l_x)
        , l_y(l_y)
        , r_x(r_x)
        , r_y(r_y)
    {
    }

    Rectangle(const CefRect& rect)
        : l_x(rect.x)
        , l_y(rect.y)
        , r_x(rect.x + rect.width)
        , r_y(rect.y + rect.height)
    {
    }

    int l_x;
    int l_y;
    int r_x;
    int r_y;

    bool invalid = false;
};

bool is_full_frame(const Rectangle& rect, int width, int height);

bool merge_rectangles(std::vector<Rectangle>& rects, int width, int height);

} // namespace caspar::html