/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 * Author: Mint de Wit mint@araminta.dev
 */

#pragma once

#include <tbb/scalable_allocator.h>

namespace caspar { namespace dmf_mxl {

template <typename T = uint16_t>
struct ARGBPixel
{
    T R;
    T G;
    T B;
    T A;
};

// template <typename T = uint16_t>
// void row_to_v210(ARGBPixel<T> const* src, int pixel_count, const std::vector<int32_t>& color_matrix, __m128i* dest);
void do_row_to_v210(ARGBPixel<uint8_t> const*   src,
                    int                         pixel_count,
                    const std::vector<int32_t>& color_matrix,
                    __m128i*                    dest);
void do_row_to_v210(ARGBPixel<uint16_t> const*  src,
                    int                         pixel_count,
                    const std::vector<int32_t>& color_matrix,
                    __m128i*                    dest);

}} // namespace caspar::dmf_mxl