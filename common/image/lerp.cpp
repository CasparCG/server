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

#include "lerp.h"

#include "../utility/types.h"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include <cassert>
#include <intrin.h>
#include <functional>

using namespace std::tr1::placeholders;

namespace caspar{
namespace common{
namespace image{

static const size_t STRIDE = sizeof(__m128i)*4;

void DolerpParallel(const tbb::blocked_range<size_t>& r, const std::tr1::function<void(void*, const void*, const void*, float, size_t)>& func, void* dest, const void* source1, const void* source2, float alpha)
{
	size_t offset = r.begin()*STRIDE;
	size_t size = r.size()*STRIDE;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source1) + offset, reinterpret_cast<const s8*>(source2) + offset, alpha, size);
}

void lerpParallel(const std::tr1::function<void(void*, const void*, const void*, float, size_t)>& func, void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/STRIDE), std::bind(&DolerpParallel, std::placeholders::_1, func, dest, source1, source2, alpha));	
}

lerp_fun get_lerp_fun(SIMD simd)
{
	if(simd >= SSE2)
		return lerpParallel_SSE2;
	else
		return lerpParallel_REF;
}

void lerp_SSE2(void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	static const u32 PSD = 64;
	
	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);
	static const __m128i round = _mm_set1_epi16(128);

	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % STRIDE == 0);
	assert(alpha >= 0.0 && alpha <= 1.0);

	const __m128i* source128_1 = reinterpret_cast<const __m128i*>(source1);
	const __m128i* source128_2 = reinterpret_cast<const __m128i*>(source2);
	__m128i* dest128 = reinterpret_cast<__m128i*>(dest);

	__m128i s = _mm_setzero_si128();
	__m128i d = _mm_setzero_si128();
	const __m128i a = _mm_set1_epi16(static_cast<u8>(alpha*256.0f+0.5f));
	
	__m128i drb, dga, srb, sga;
	
	for (size_t k = 0, length = size/STRIDE; k < length; ++k)
	{		
		_mm_prefetch(reinterpret_cast<const char*>(source128_1 + PSD), _MM_HINT_NTA);	
		_mm_prefetch(reinterpret_cast<const char*>(source128_2 + PSD), _MM_HINT_NTA);
		// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

		for(int n = 0; n < 4; ++n, ++dest128, ++source128_1, ++source128_2)
		{
			// r = d + (s-d)*alpha/256
			s = _mm_load_si128(source128_1);	// AABBGGRR
			d = _mm_load_si128(source128_2);	// AABBGGRR

			srb = _mm_and_si128(lomask, s);		// 00BB00RR		// unpack
			sga = _mm_srli_epi16(s, 8);			// AA00GG00		// unpack
			
			drb = _mm_and_si128(lomask, d);		// 00BB00RR		// unpack
			dga = _mm_srli_epi16(d, 8);			// AA00GG00		// unpack

			srb = _mm_sub_epi16(srb, drb);		// BBBBRRRR		// sub
			srb = _mm_mullo_epi16(srb, a);		// BBBBRRRR		// mul
			srb = _mm_add_epi16(srb, round);
			
			sga = _mm_sub_epi16(sga, dga);		// AAAAGGGG		// sub
			sga = _mm_mullo_epi16(sga, a);		// AAAAGGGG		// mul
			sga = _mm_add_epi16(sga, round);

			srb = _mm_srli_epi16(srb, 8);		// 00BB00RR		// prepack and div
			sga = _mm_andnot_si128(lomask, sga);// AA00GG00		// prepack and div

			srb = _mm_or_si128(srb, sga);		// AABBGGRR		// pack

			srb = _mm_add_epi8(srb, d);			// AABBGGRR		// add		there is no overflow(R.N)

			_mm_stream_si128(dest128, srb);
		}
	}
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void lerpParallel_SSE2(void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	lerpParallel(&lerp_SSE2, dest, source1, source2, alpha, size);
}

void lerp_REF(void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % 4 == 0);
	assert(alpha >= 0.0f && alpha <= 1.0f);

	const u8* source8_1 = reinterpret_cast<const u8*>(source1);
	const u8* source8_2 = reinterpret_cast<const u8*>(source2);
	u8* dest8 = reinterpret_cast<u8*>(dest);

	u8 a = static_cast<u8>(alpha*256.0f);
	for(size_t n = 0; n < size; n+=4)
	{
		// s
		u32 sr = source8_1[n+0];
		u32 sg = source8_1[n+1];
		u32 sb = source8_1[n+2];
		u32 sa = source8_1[n+3];

		// d
		u32 dr = source8_2[n+0];
		u32 dg = source8_2[n+1];
		u32 db = source8_2[n+2];
		u32 da = source8_2[n+3];

		//dest8[n+0] = dr + ((sr-dr)*a)/256;
		//dest8[n+1] = dg + ((sg-dg)*a)/256;
		//dest8[n+2] = db + ((sb-db)*a)/256;
		//dest8[n+3] = da + ((sa-da)*a)/256;

		dest8[n+0] = static_cast<u8>( dr + int(float((sr-dr)*a)/256.0f+0.5f));
		dest8[n+1] = static_cast<u8>( dg + int(float((sg-dg)*a)/256.0f+0.5f));
		dest8[n+2] = static_cast<u8>( db + int(float((sb-db)*a)/256.0f+0.5f));
		dest8[n+3] = static_cast<u8>(da + int(float((sa-da)*a)/256.0f+0.5f));

	}
}

void lerpParallel_REF(void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	lerpParallel(&lerp_REF, dest, source1, source2, alpha, size);
}

} // namespace image
} // namespace common
} // namespace caspar


