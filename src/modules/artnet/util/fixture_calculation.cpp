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
 * Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "fixture_calculation.h"

namespace caspar { namespace artnet {

rect compute_rect(box fixtureBox, int index, int count)
{
    // Calculates the corners of an axis-aligned rectangle that is part of a fixture chain
    // The count represents how many fixtures exist in the box and the index which one to calculate

    float fixture_width = fixtureBox.width / (float)count;

    float x0 = fixtureBox.x + (float)index * fixture_width;
    float x1 = x0 + fixture_width;
    float y0 = fixtureBox.y;
    float y1 = y0 + fixtureBox.height;

    return rect{
        {x0, y0},
        {x1, y0},
        {x1, y1},
        {x0, y1},
    };
}

color average_color(const core::const_frame& frame, rect& rectangle)
{
    int width  = (int)frame.width();
    int height = (int)frame.height();

    float x_values[] = {rectangle.p1.x, rectangle.p2.x, rectangle.p3.x, rectangle.p4.x};
    float y_values[] = {rectangle.p1.y, rectangle.p2.y, rectangle.p3.y, rectangle.p4.y};

    // Sort the points by y value, then by x value if y is equal
    for (int i = 0; i < 3; i++) {
        for (int j = 3; j > i; j--) {
            if (y_values[j] > y_values[j - 1])
                continue;
            if (y_values[j] < y_values[j - 1]) {
                float x = x_values[j];
                float y = y_values[j];

                x_values[j] = x_values[j - 1];
                y_values[j] = y_values[j - 1];

                x_values[j - 1] = x;
                y_values[j - 1] = y;

                continue;
            }

            if (x_values[j] < x_values[j - 1]) {
                float x = x_values[j];

                x_values[j]     = x_values[j - 1];
                x_values[j - 1] = x;
            }
        }
    }

    // Below is a rasterization algorithm that goes through the pixels in rectangle
    // and calculates the average color

    // Which lines to use for the rasterization
    // in the format [a, b, c, d] => a -> b, c -> d
    // the numbers are indices into the x_values, y_values arrays
    const int indices[3][4] = {
        {0, 1, 0, 2}, // Line 1, Line 2
        {0, 2, 1, 3}, // Line 2, Line 3
        {1, 3, 2, 3}, // Line 3, Line 4
    };

    // The y values of the top and bottom of the rectangle, clamped to the image size
    int y_min = std::max(0, std::min(height - 1, (int)y_values[0]));
    int y_max = std::max(0, std::min(height - 1, (int)y_values[3]));

    const array<const std::uint8_t>& values    = frame.image_data(0);
    const std::uint8_t*              value_ptr = values.data();

    // Total color values, as well as the number of pixels in the rectangle
    // used to calculate the average without loss of precision
    unsigned long long tr = 0;
    unsigned long long tg = 0;
    unsigned long long tb = 0;

    unsigned long long count = 0;

    // Go through the vertical lines of the rectangle, and then through the pixels in the line
    // that are inside the rectangle
    for (int y = y_min; y <= y_max; y++) {
        // Determine which lines to use for the rasterization, if one line has passed we should use the next one
        int index = 0;
        if (y >= (int)y_values[1])
            index = 1;
        if (y >= (int)y_values[2])
            index = 2;

        // The x and y values of the first line
        float ax1 = x_values[indices[index][0]];
        float ay1 = y_values[indices[index][0]];

        float bx1 = x_values[indices[index][1]];
        float by1 = y_values[indices[index][1]];

        // The x and y values of the second line
        float ax2 = x_values[indices[index][2]];
        float ay2 = y_values[indices[index][2]];

        float bx2 = x_values[indices[index][3]];
        float by2 = y_values[indices[index][3]];

        int x1 = 0;
        int x2 = width - 1;

        // If the lines are horizontal, we can skip the calculations
        // This only happens if the box is oriented in 90 degree increments
        if (by2 != ay2) {
            // The slopes of the lines
            float d1 = (bx1 - ax1) / (by1 - ay1);
            float d2 = (bx2 - ax2) / (by2 - ay2);

            // The x values of the lines at the current y value
            auto x1_ = (int)(ax1 + ((float)y - ay1) * d1);
            auto x2_ = (int)(ax2 + ((float)y - ay2) * d2);

            // The clamped x values
            x1 = std::max(0, std::min(width - 1, x1_));
            x2 = std::max(0, std::min(width - 1, x2_));
        }

        // The left and right x values
        int min_x = std::min(x1, x2);
        int max_x = std::max(x1, x2);

        // Go through the pixels in the line
        for (int x = min_x; x <= max_x; x++) {
            int                 pos      = y * width + x;
            const std::uint8_t* base_ptr = value_ptr + pos * 4;

            float a = (float)base_ptr[3] / 255.0f;

            float r = (float)base_ptr[2] * a;
            float g = (float)base_ptr[1] * a;
            float b = (float)base_ptr[0] * a;

            tr += (unsigned long long)(r);
            tg += (unsigned long long)(g);
            tb += (unsigned long long)(b);

            count++;
        }
    }

    color c{(std::uint8_t)(tr / count), (std::uint8_t)(tg / count), (std::uint8_t)(tb / count)};

    return c;
}

}} // namespace caspar::artnet