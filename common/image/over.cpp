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

#include "over.h"

#include "../hardware/cpuid.h"
#include "../utility/types.h"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include <cassert>
#include <intrin.h>
#include <functional>

using namespace std::placeholders;

namespace caspar { namespace common { namespace image	{

static const size_t STRIDE = sizeof(__m128i)*4;

void Dopre_overParallel(const tbb::blocked_range<size_t>& r, const std::tr1::function<void(void*, const void*, const void*, size_t)>& func, void* dest, const void* source1, const void* source2)
{
	size_t offset = r.begin()*STRIDE;
	size_t size = r.size()*STRIDE;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source1) + offset, reinterpret_cast<const s8*>(source2) + offset, size);
}

void pre_overParallel(const std::tr1::function<void(void*, const void*, const void*, size_t)>& func, void* dest, const void* source1, const void* source2, size_t size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/STRIDE), std::bind(&Dopre_overParallel, std::placeholders::_1, func, dest, source1, source2));	
}

pre_over_fun get_pre_over_fun(SIMD simd)
{
	if(simd >= SSE2)
		return pre_overParallel_SSE2;
	else
		return pre_overParallel_REF;
}

// this function performs precise calculations
void pre_over_SSE2(void* dest, const void* source1, const void* source2, size_t size)
{
	static const u32 PSD = 64;

	static const __m128i round = _mm_set1_epi16(128);
	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);

	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % STRIDE == 0);

	const __m128i* source128_1 = reinterpret_cast<const __m128i*>(source1);
	const __m128i* source128_2 = reinterpret_cast<const __m128i*>(source2);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	
	
	__m128i d, s, a, rb, ag, t;

	// TODO: dynamic prefetch schedluing distance? needs to be optimized (R.N)

	for(size_t k = 0, length = size/STRIDE; k < length; ++k)	
	{
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128_1+PSD), _MM_HINT_NTA);
		_mm_prefetch(reinterpret_cast<const s8*>(source128_2+PSD), _MM_HINT_NTA);	

		// work on entire cacheline before next prefetch
		for(int n = 0; n < 4; ++n, ++dest128, ++source128_1, ++source128_2)
		{
			// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

			// TODO: load entire cacheline at the same time? are there enough registers? 32 bit mode (special compile for 64bit?) (R.N)
			s = _mm_load_si128(source128_1);		// AABGGRR
			d = _mm_load_si128(source128_2);		// AABGGRR
						
			// PRELERP(S, D) = S+D - ((S*D[A]+0x80)>>8)+(S*D[A]+0x80))>>8
			// T = S*D[A]+0x80 => PRELERP(S,D) = S+D - ((T>>8)+T)>>8

			// set alpha to lo16 from dest_
			a = _mm_srli_epi32(d, 24);			// 000000AA	
			rb = _mm_slli_epi32(a, 16);			// 00AA0000
			a = _mm_or_si128(rb, a);			// 00AA00AA

			rb = _mm_and_si128(lomask, s);		// 00BB00RR		
			rb = _mm_mullo_epi16(rb, a);		// BBBBRRRR	
			rb = _mm_add_epi16(rb, round);		// BBBBRRRR
			t = _mm_srli_epi16(rb, 8);			
			t = _mm_add_epi16(t, rb);
			rb = _mm_srli_epi16(t, 8);			// 00BB00RR	

			ag = _mm_srli_epi16(s, 8); 			// 00AA00GG		
			ag = _mm_mullo_epi16(ag, a);		// AAAAGGGG		
			ag = _mm_add_epi16(ag, round);
			t = _mm_srli_epi16(ag, 8);
			t = _mm_add_epi16(t, ag);
			ag = _mm_andnot_si128(lomask, t);	// AA00GG00		
					
			rb = _mm_or_si128(rb, ag);			// AABGGRR		pack
					
			rb = _mm_sub_epi8(s, rb);			// sub S-[(D[A]*S)/255]
			d = _mm_add_epi8(d, rb);			// add D+[S-(D[A]*S)/255]

			_mm_stream_si128(dest128, d);
		}
	}	
	_mm_mfence();	//ensure last WC buffers get flushed to memory		
}

void pre_overParallel_SSE2(void* dest, const void* source1, const void* source2, size_t size)
{
	pre_overParallel(&pre_over_SSE2, dest, source1, source2, size);	
}

// TODO: optimize
void pre_over_REF(void* dest, const void* source1, const void* source2, size_t size)
{	
	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % 4 == 0);

	const u8* source8_1 = reinterpret_cast<const u8*>(source1);
	const u8* source8_2 = reinterpret_cast<const u8*>(source2);
	u8* dest8 = reinterpret_cast<u8*>(dest);

	for(size_t n = 0; n < size; n+=4)
	{
		u32 r1 = source8_1[n+0];
		u32 g1 = source8_1[n+1];
		u32 b1 = source8_1[n+2];
		u32 a1 = source8_1[n+3];

		u32 r2 = source8_2[n+0];
		u32 g2 = source8_2[n+1];
		u32 b2 = source8_2[n+2];
		u32 a2 = source8_2[n+3];

		dest8[n+0] = static_cast<u8>(r2 + r1 - (a2*r1)/255);
		dest8[n+1] = static_cast<u8>(g2 + g1 - (a2*g1)/255);
		dest8[n+2] = static_cast<u8>(b2 + b1 - (a2*b1)/255);
		dest8[n+3] = static_cast<u8>(a2 + a1 - (a2*a1)/255);
	}
}

void pre_overParallel_REF(void* dest, const void* source1, const void* source2, size_t size)
{
	pre_overParallel(&pre_over_REF, dest, source1, source2, size);	
}

}}}


