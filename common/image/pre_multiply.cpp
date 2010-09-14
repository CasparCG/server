/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "../stdafx.h"

#include "pre_multiply.h"

#include "../utility/types.h"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include <cassert>
#include <intrin.h>
#include <functional>

using namespace std::tr1::placeholders;

namespace caspar{ namespace common{ namespace image{

static const size_t STRIDE = sizeof(__m128i)*4;

void Dopre_multiplyParallel(const tbb::blocked_range<size_t>& r, const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source)
{
	size_t offset = r.begin()*STRIDE;
	size_t size = r.size()*STRIDE;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source) + offset, size);
}

void pre_multiplyParallel(const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source, size_t size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/STRIDE), std::bind(&Dopre_multiplyParallel, std::placeholders::_1, func, dest, source));	
}

pre_multiply_fun get_pre_multiply_fun(SIMD simd)
{
	if(simd >= SSE2)
		return pre_multiplyParallel_SSE2;
	else
		return pre_multiplyParallel_REF;
}

// this function performs precise calculations
void pre_multiply_SSE2(void* dest, const void* source, size_t size)
{
	static const u32 PSD = 64;

	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);
	static const __m128i amask = _mm_set1_epi32(0xFF000000);
	static const __m128i round = _mm_set1_epi16(128);	

	assert(source != NULL && dest != NULL);
	assert(size % STRIDE == 0);
	
	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	

	__m128i s, rb, ag, a, t;		
	
	for(size_t k = 0, length = size/STRIDE; k != length; ++k)	
	{
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128 + PSD), _MM_HINT_NTA);

		// prefetch fetches entire cacheline (512bit). work on entire cacheline before next prefetch. 512/128 = 4, unroll four times = 16 pixels

		// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

		for(int n = 0; n < 4; ++n, ++dest128, ++source128)
		{
			s = _mm_load_si128(source128);		// AABBGGRR

			// set alpha to lo16 from source
			rb = _mm_srli_epi32(s, 24);			// 000000AA
			a = _mm_slli_epi32(rb, 16);			// 00AA0000
			a = _mm_or_si128(rb, a);			// 00AA00AA

			rb = _mm_and_si128(lomask, s);		// 00BB00RR		
			rb = _mm_mullo_epi16(rb, a);		// BBBBRRRR	
			rb = _mm_add_epi16(rb, round);		// BBBBRRRR
			t = _mm_srli_epi16(rb, 8);			// 00BB00RR	
			t = _mm_add_epi16(t, rb);
			rb = _mm_srli_epi16(t, 8);

			ag = _mm_srli_epi16(s, 8); 			// 00AA00GG		
			ag = _mm_mullo_epi16(ag, a);		// AAAAGGGG		
			ag = _mm_add_epi16(ag, round);
			t = _mm_srli_epi16(ag, 8);
			t = _mm_add_epi16(t, ag);
			ag = _mm_andnot_si128(lomask, t);	// AA00GG00		
					
			a = _mm_or_si128(rb, ag);			// XXBBGGRR
			a = _mm_andnot_si128(amask, a);		// 00BBGGRR

			s = _mm_and_si128(amask, s);		// AA000000

			s = _mm_or_si128(a, s);				// AABBGGRR		pack

			// TODO: store entire cache line at the same time (write-combining => burst)? are there enough registers? 32 bit mode (special compile for 64bit?) (R.N)
			_mm_stream_si128(dest128, s);
		}		
	}
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void pre_multiplyParallel_SSE2(void* dest, const void* source1, size_t size)
{
	pre_multiplyParallel(&pre_multiply_SSE2, dest, source1, size);
}

void pre_multiply_REF(void* dest, const void* source, size_t size)
{
	assert(source != NULL && dest != NULL);
	assert(size % 4 == 0);

	const u8* source8 = reinterpret_cast<const u8*>(source);
	u8* dest8 = reinterpret_cast<u8*>(dest);

	for(size_t n = 0; n < size; n+=4)
	{
		u32 r = source8[n+0];
		u32 g = source8[n+1];
		u32 b = source8[n+2];
		u32 a = source8[n+3];

		dest8[n+0] = static_cast<u8>((r*a)/255);
		dest8[n+1] = static_cast<u8>((g*a)/255);
		dest8[n+2] = static_cast<u8>((b*a)/255);
		dest8[n+3] = static_cast<u8>(a);
	}
}

void pre_multiplyParallel_REF(void* dest, const void* source1,  size_t size)
{
	pre_multiplyParallel(&pre_multiply_REF, dest, source1, size);
}

}}}
