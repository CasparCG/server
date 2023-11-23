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

#include "rectangles.h"

#include <algorithm>

namespace caspar::html {

bool is_full_frame(const Rectangle& rect, int width, int height)
{
    return rect.l_x == 0 && rect.l_y == 0 && rect.r_x == width && rect.r_y == height;
}

bool are_any_full_frame(const std::vector<Rectangle>& rects, int width, int height)
{
    return std::any_of(
        rects.begin(), rects.end(), [&](const Rectangle& rect) { return is_full_frame(rect, width, height); });
}

bool should_merge_rectanges(const Rectangle& a, const Rectangle& b)
{
    int x_tolerance = 100; // Use some collision tolerance, so that we don't do too many memcpy per line

    // If one rectangle is on left side of other
    if (a.l_x > b.r_x + x_tolerance || b.l_x > a.r_x + x_tolerance)
        return false;

    // If one rectangle is above other
    if (a.r_y < b.l_y || b.r_y < a.l_y)
        return false;

    return true;
}

bool is_invalid(const Rectangle& rect) { return rect.invalid; }

struct
{
    bool operator()(const Rectangle& a, const Rectangle& b) const
    {
        if (a.l_y == b.l_y) {
            // Secondary sort by x
            return a.l_x < b.l_x;
        } else {
            return a.l_y < b.l_y;
        }
    }
} compare_slices;

bool merge_rectangles(std::vector<Rectangle>& rects, int width, int height)
{
    // If there is 0 or 1 rectangles, there is nothing to merge
    if (rects.size() <= 1)
        return false;

    // If any are full-frame, then that is the only rectangle of interest
    bool full_frame = are_any_full_frame(rects, width, height);
    if (full_frame) {
        rects.clear();
        rects.emplace_back(0, 0, width, height);
        return true;
    }

    std::sort(rects.begin(), rects.end(), compare_slices);

    bool has_changes = false;

    // Iterate through rectangles, merging any that overlap.
    for (auto first = rects.begin(); first != rects.end(); ++first) {
        // Skip over any slices that have been merged.
        if (first->invalid)
            continue;

        // Iterate through all following slices
        for (auto second = first + 1; second != rects.end(); ++second) {
            // Skip over any slices that have been merged.
            if (second->invalid)
                continue;

            if (should_merge_rectanges(*first, *second)) {
                first->l_x = std::min(first->l_x, second->l_x);
                first->l_y = std::min(first->l_y, second->l_y);
                first->r_x = std::max(first->r_x, second->r_x);
                first->r_y = std::max(first->r_y, second->r_y);

                // Mark the second slice as having been merged.
                second->invalid = true;
                has_changes     = true;
            }
        }
    }

    if (!has_changes)
        return false;

    // Snip out any rectangles that are invalid
    rects.erase(remove_if(rects.begin(), rects.end(), is_invalid), rects.end());

    return true;
}

} // namespace caspar::html