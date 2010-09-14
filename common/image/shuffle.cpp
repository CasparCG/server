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

#include "shuffle.h"

#include "../utility/types.h"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include <cassert>
#include <intrin.h>
#include <functional>

using namespace std::tr1::placeholders;

namespace caspar{ namespace common{ namespace image{

static const size_t STRIDE = sizeof(__m128i)*4;

void DoshuffleParallel(const tbb::blocked_range<size_t>& r, const std::tr1::function<void(void*, const void*, size_t, const u8, const u8, const u8, const u8)>& func, void* dest, const void* source, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	size_t offset = r.begin()*STRIDE;
	size_t size = r.size()*STRIDE;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source) + offset, size, red, green, blue, alpha);
}

void shuffleParallel(const std::tr1::function<void(void*, const void*, size_t, const u8, const u8, const u8, const u8)>& func, void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/STRIDE), std::bind(&DoshuffleParallel, std::placeholders::_1, func, dest, source, red, green, blue, alpha));	
}

shuffle_fun get_shuffle_fun(SIMD simd)
{
	if(simd >= SSSE3)
		return shuffleParallel_SSSE3;
	else if(simd >= SSE2)
		return shuffleParallel_SSE2;
	else
		return shuffleParallel_REF;
}

void shuffle_SSSE3(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	static const unsigned int PSD = 64;	

	assert(source != NULL && dest != NULL);
	assert(red > -1 && red < 4 && green > -1 && green < 4 && blue > -1 && blue < 4 && alpha > -1 && alpha < 4 && "Invalid mask");
	assert(size % STRIDE == 0);

	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	

	__m128i reg0 = _mm_setzero_si128();	
	__m128i reg1 = _mm_setzero_si128();	
	__m128i reg2 = _mm_setzero_si128();	
	__m128i reg3 = _mm_setzero_si128();	

	const __m128i mask128 = _mm_set_epi8(alpha+12, blue+12, green+12, red+12, alpha+8, blue+8, green+8, red+8, alpha+4, blue+4, green+4, red+4, alpha, blue, green, red);

	for(size_t k = 0, length = size/STRIDE; k < length; ++k)	
	{
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128 + PSD), _MM_HINT_NTA);

		// work on entire cacheline before next prefetch

		// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

		reg0 = _mm_load_si128(source128++);	
		reg1 = _mm_load_si128(source128++);	

		_mm_stream_si128(dest128++, _mm_shuffle_epi8(reg0, mask128));

		reg2 = _mm_load_si128(source128++);	

		_mm_stream_si128(dest128++, _mm_shuffle_epi8(reg1, mask128));

		reg3 = _mm_load_si128(source128++);	
		
		_mm_stream_si128(dest128++, _mm_shuffle_epi8(reg2, mask128));	
		_mm_stream_si128(dest128++, _mm_shuffle_epi8(reg3, mask128));		
	}
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void shuffleParallel_SSSE3(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	shuffleParallel(&shuffle_SSSE3, dest, source, size, red, green, blue, alpha);
}

// TODO: should be optimized for different combinations (R.N)
void shuffle_SSE2(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	static const size_t stride = sizeof(__m128i)*4;
	static const u32 PSD = 64;

	static const __m128i himask	= _mm_set1_epi32(0xFF000000);	
	static const __m128i lomask	= _mm_set1_epi32(0x000000FF);
	
	assert(source != NULL && dest != NULL);
	assert(red > -1 && red < 4 && green > -1 && green < 4 && blue > -1 && blue < 4 && alpha > -1 && alpha < 4);
	assert(size % stride == 0);

	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	

	__m128i s, m0, m1, r;

	const int shft0 = (red)*8;
	const int shft1 = (green)*8;
	const int shft2 = (3-blue)*8;
	const int shft3 = (3-alpha)*8;

	for(int k = 0, length = size/stride; k < length; ++k)	
	{
		// TODO: dynamic prefetch schedluing distance? needs to be optimized (R.N)		
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128 + PSD), _MM_HINT_NTA);

		// work on entire cacheline before next prefetch

		// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

		for(int n = 0; n < 4; ++n, ++dest128, ++source128)
		{
			s = _mm_load_si128(source128);
			
			m0 = _mm_srli_epi32(s, shft0);
			m0 = _mm_and_si128(m0, lomask);

			m1 = _mm_srli_epi32(s, shft1);
			m1 = _mm_and_si128(m1, lomask);
			m1 = _mm_slli_epi32(m1, 8);
			
			r = _mm_or_si128(m0, m1);

			m0 = _mm_slli_epi32(s, shft2);
			m0 = _mm_and_si128(m0, himask);
			m0 = _mm_srli_epi32(m0, 8);			

			m1 = _mm_slli_epi32(s, shft3);
			m1 = _mm_and_si128(m1, himask);
			
			m0 = _mm_or_si128(m0, m1);

			r = _mm_or_si128(r, m0);

			_mm_stream_si128(dest128, r);
		}
	}
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void shuffleParallel_SSE2(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	shuffleParallel(&shuffle_SSE2, dest, source, size, red, green, blue, alpha);
}

void shuffle_REF(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	assert(source != NULL && dest != NULL);
	assert(red > -1 && red < 4 && green > -1 && green < 4 && blue > -1 && blue < 4 && alpha > -1 && alpha < 4);
	assert(size % 4 == 0);

	const u8* source8 = reinterpret_cast<const u8*>(source);
	u8*		  dest8 = reinterpret_cast<u8*>(dest);	

	for(size_t n = 0; n < size; n+=4)
	{
		u8 r = source8[n+red];
		u8 g = source8[n+green];
		u8 b = source8[n+blue];
		u8 a = source8[n+alpha];

		dest8[n+0] = r;
		dest8[n+1] = g;
		dest8[n+2] = b;
		dest8[n+3] = a;
	}
}

void shuffleParallel_REF(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	shuffleParallel(&shuffle_REF, dest, source, size, red, green, blue, alpha);
}

}}}
