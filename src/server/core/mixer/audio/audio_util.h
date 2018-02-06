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

#include <algorithm>
#include <cstdint>
#include <vector>

namespace caspar { namespace core {

template <typename T>
std::vector<int8_t> audio_32_to_24(const T& audio_data)
{
    auto size    = std::distance(std::begin(audio_data), std::end(audio_data));
    auto input8  = reinterpret_cast<const int8_t*>(&(*std::begin(audio_data)));
    auto output8 = std::vector<int8_t>();

    output8.reserve(size * 3);
    for (int n = 0; n < size; ++n) {
        output8.push_back(input8[n * 4 + 1]);
        output8.push_back(input8[n * 4 + 2]);
        output8.push_back(input8[n * 4 + 3]);
    }

    return output8;
}

template <typename T>
std::vector<int16_t> audio_32_to_16(const T& audio_data)
{
    auto size     = std::distance(std::begin(audio_data), std::end(audio_data));
    auto input32  = &(*std::begin(audio_data));
    auto output16 = std::vector<int16_t>();

    output16.reserve(size);
    for (int n = 0; n < size; ++n)
        output16.push_back((input32[n] >> 16) & 0xFFFF);

    return output16;
}

}} // namespace caspar::core