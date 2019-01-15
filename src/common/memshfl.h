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

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <tmmintrin.h>
#endif

namespace caspar {

static void* aligned_memshfl(void* dest, const void* source, size_t count, int m1, int m2, int m3, int m4)
{
    __m128i*       dest128   = reinterpret_cast<__m128i*>(dest);
    const __m128i* source128 = reinterpret_cast<const __m128i*>(source);

    count /= 16; // 128 bit

    const __m128i mask128 = _mm_set_epi32(m1, m2, m3, m4);
    for (size_t n = 0; n < count / 4; ++n) {
        __m128i xmm0 = _mm_load_si128(source128++);
        __m128i xmm1 = _mm_load_si128(source128++);
        __m128i xmm2 = _mm_load_si128(source128++);
        __m128i xmm3 = _mm_load_si128(source128++);

        _mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm0, mask128));
        _mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm1, mask128));
        _mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm2, mask128));
        _mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm3, mask128));
    }
    return dest;
}

} // namespace caspar
