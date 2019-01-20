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
 * Author: Helge Norberg, helge.norberg@svt.se.com
 */

#include "image_algorithms.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace caspar { namespace image {

std::vector<std::pair<int, int>> get_line_points(int num_pixels, double angle_radians)
{
    std::vector<std::pair<int, int>> line_points;
    line_points.reserve(num_pixels);

    double delta_x       = std::cos(angle_radians);
    double delta_y       = -std::sin(angle_radians); // In memory is revered
    double max_delta     = std::max(std::abs(delta_x), std::abs(delta_y));
    double amplification = 1.0 / max_delta;
    delta_x *= amplification;
    delta_y *= amplification;

    for (int i = 1; i <= num_pixels; ++i)
        line_points.push_back(std::make_pair(static_cast<int>(std::floor(delta_x * static_cast<double>(i) + 0.5)),
                                             static_cast<int>(std::floor(delta_y * static_cast<double>(i) + 0.5))));

    return std::move(line_points);
}

}} // namespace caspar::image
