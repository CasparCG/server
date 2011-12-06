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

#include <intrin.h>

#include <assert.h>

#include <tbb/parallel_for.h>

namespace caspar {

namespace internal {

static void* fast_memshfl(void* dest, const void* source, size_t count, int m1, int m2, int m3, int m4)
{
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	
	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);

	count /= 16; // 128 bit

	__m128i xmm0, xmm1, xmm2, xmm3;

	const __m128i mask128 = _mm_set_epi32(m1, m2, m3, m4);
	for(size_t n = 0; n < count/4; ++n)
	{
		xmm0 = _mm_load_si128(source128++);	
		xmm1 = _mm_load_si128(source128++);	
		xmm2 = _mm_load_si128(source128++);	
		xmm3 = _mm_load_si128(source128++);	

		_mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm0, mask128));
		_mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm1, mask128));
		_mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm2, mask128));
		_mm_stream_si128(dest128++, _mm_shuffle_epi8(xmm3, mask128));
	}
	return dest;
}

}

static void* fast_memshfl(void* dest, const void* source, size_t count, int m1, int m2, int m3, int m4)
{   
	tbb::affinity_partitioner ap;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, count/128), [&](const tbb::blocked_range<size_t>& r)
	{       
		internal::fast_memshfl(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128, m1, m2, m3, m4);   
	}, ap);

	return dest;
}


}