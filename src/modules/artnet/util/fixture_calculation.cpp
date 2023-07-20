/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "fixture_calculation.h"

#define M_PI           3.14159265358979323846  /* pi */

namespace caspar {
    namespace artnet {

        rect compute_rect(box fixtureBox, int index, int count)
        {
            auto f_count = (float) count;
            auto f_index = (float) index;

            float x = fixtureBox.x;
            float y = fixtureBox.y;

            float width  = fixtureBox.width;
            float height = fixtureBox.height;

            float rotation = fixtureBox.rotation;
            double angle = M_PI * rotation / 180.0f;

            double sin_ = sin(angle);
            double cos_ = cos(angle);

            float dx = width / (2 * f_count);
            float dy = height / 2.0f;

            float od = (2 * f_index - f_count + 1) * dx;

            double ox = x + od * cos_;
            double oy = y + od * sin_;

            point p1 {
                static_cast<float>(ox + -dx *  cos_ + -dy * -sin_),
                static_cast<float>(oy + -dx *  sin_ + -dy *  cos_),
            };

            point p2 {
                static_cast<float>(ox +  dx *  cos_ + -dy * -sin_),
                static_cast<float>(oy +  dx *  sin_ + -dy *  cos_),
            };

            point p3 {
                static_cast<float>(ox +  dx *  cos_ +  dy * -sin_),
                static_cast<float>(oy +  dx *  sin_ +  dy *  cos_),
            };

            point p4 {
                static_cast<float>(ox + -dx *  cos_ +  dy * -sin_),
                static_cast<float>(oy + -dx *  sin_ +  dy *  cos_),
            };

            rect rectangle {
                p1,
                p2,
                p3,
                p4
            };

            return rectangle;
        }

        color average_color(const core::const_frame& frame, rect& rectangle) {
            int width = (int) frame.width();
            int height = (int) frame.height();

            float x_values[] = {
                rectangle.p1.x,
                rectangle.p2.x,
                rectangle.p3.x,
                rectangle.p4.x
            };
            float y_values[] = {
                rectangle.p1.y,
                rectangle.p2.y,
                rectangle.p3.y,
                rectangle.p4.y
            };

            for (int i = 0; i < 3; i++) {
                for (int j = 3; j > i; j--) {
                    if (y_values[j] > y_values[j - 1]) continue;
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

                        x_values[j] = x_values[j - 1];
                        x_values[j - 1] = x;
                    }
                }
            }

            const int indices[3][4] = {
                {0, 1, 0, 2},
                {0, 2, 1, 3},
                {1, 3, 2, 3},
            };

            int y_min = std::max(0, std::min(height - 1, (int) y_values[0]));
            int y_max = std::max(0, std::min(height - 1, (int) y_values[3]));

            const array<const std::uint8_t>& values = frame.image_data(0);
            const std::uint8_t* value_ptr = values.data();

            unsigned long long tr = 0;
            unsigned long long tg = 0;
            unsigned long long tb = 0;

            unsigned long long count = 0;

            for (int y = y_min; y <= y_max; y++) {
                int index = 0;
                if (y >= (int) y_values[1]) index = 1;
                if (y >= (int) y_values[2]) index = 2;

                float ax1 = x_values[indices[index][0]];
                float ay1 = y_values[indices[index][0]];

                float bx1 = x_values[indices[index][1]];
                float by1 = y_values[indices[index][1]];

                float ax2 = x_values[indices[index][2]];
                float ay2 = y_values[indices[index][2]];

                float bx2 = x_values[indices[index][3]];
                float by2 = y_values[indices[index][3]];

                float d1 = (bx1 - ax1) / (by1 - ay1);
                float d2 = (bx2 - ax2) / (by2 - ay2);

                auto x1 = std::max(0, std::min(width - 1, (int) (ax1 + ((float) y - ay1) * d1)));
                auto x2 = std::max(0, std::min(width - 1, (int) (ax2 + ((float) y - ay2) * d2)));

                int min_x = std::min(x1, x2);
                int max_x = std::max(x1, x2);

                for (int x = min_x; x <= max_x; x++) {
                    int pos = y * width + x;
                    const std::uint8_t* base_ptr = value_ptr + pos * 4;

                    float a = (float) base_ptr[3] / 255.0f;

                    float r = (float) base_ptr[2] * a;
                    float g = (float) base_ptr[1] * a;
                    float b = (float) base_ptr[0] * a;

                    tr += (unsigned long long) (r);
                    tg += (unsigned long long) (g);
                    tb += (unsigned long long) (b);

                    count++;
                }
            }

            color c {
                (std::uint8_t) (tr / count),
                (std::uint8_t) (tg / count),
                (std::uint8_t) (tb / count)
            };

            return c;
        }

    }
} // namespace caspar::artnet