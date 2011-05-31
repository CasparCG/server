#include "..\..\stdafx.h"

#include "Over.hpp"

#include <intrin.h>
#include "../Types.hpp"

namespace caspar{
namespace utils {
namespace image	{

PreOverFun GetPreOverFun(SIMD simd)
{
	if(simd >= SSE2)
		return PreOver_SSE2;
	else
		return PreOver_REF;
}

// this function performs precise calculations
void PreOver_SSE2(void* dest, const void* source1, const void* source2, size_t size)
{
	static const size_t stride = sizeof(__m128i)*4;
	static const u32 PSD = 64;

	static const __m128i round = _mm_set1_epi16(128);
	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);

	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % stride == 0);

	const __m128i* source128_1 = reinterpret_cast<const __m128i*>(source1);
	const __m128i* source128_2 = reinterpret_cast<const __m128i*>(source2);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	
	
	__m128i d, s, a, rb, ag, t;

	// TODO: dynamic prefetch schedluing distance? needs to be optimized (R.N)

	for(size_t k = 0, length = size/stride; k < length; ++k)	
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
			t = _mm_srli_epi16(rb, 8);			// 00BB00RR	
			t = _mm_add_epi16(t, rb);
			rb = _mm_srli_epi16(t, 8);

			ag = _mm_srli_epi16(s, 8); 			// 00AA00GG		
			ag = _mm_mullo_epi16(ag, a);		// AAAAGGGG		
			ag = _mm_add_epi16(ag, round);
			t = _mm_srli_epi16(ag, 8);
			t = _mm_add_epi16(t, ag);
			ag = _mm_andnot_si128(lomask, t);	// AA00GG00		
					
			rb = _mm_or_si128(rb, ag);			// AABGGRR		pack
					
			rb = _mm_sub_epi8(s, rb);			// sub S-[(D[A]*S)/255]
			d = _mm_add_epi8(d, rb);			// add D+[S-(D[A]*S)/255]

			_mm_store_si128(dest128, d);
		}
	}		
}


void PreOver_FastSSE2(void* dest, const void* source1, const void* source2, size_t size)
{
	static const size_t stride = sizeof(__m128i)*4;
	static const u32 PSD = 64;

	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);

	assert(source1 != NULL && source2 != NULL && dest != NULL);
	assert(size % stride == 0);

	const __m128i* source128_1 = reinterpret_cast<const __m128i*>(source1);
	const __m128i* source128_2 = reinterpret_cast<const __m128i*>(source2);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);		

	__m128i d, s, a, rb, ag;
	
	// TODO: dynamic prefetch schedluing distance? needs to be optimized (R.N)
	for(int k = 0, length = size/stride; k < length; ++k)	
	{
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128_1+PSD), _MM_HINT_NTA);
		_mm_prefetch(reinterpret_cast<const s8*>(source128_2+PSD), _MM_HINT_NTA);	

		//work on entire cacheline before next prefetch
		for(int n = 0; n < 4; ++n, ++dest128, ++source128_1, ++source128_2)
		{
			// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

			s = _mm_load_si128(source128_1);		// AABGGRR
			d = _mm_load_si128(source128_2);		// AABGGRR
						
			// set alpha to lo16 from dest_
			rb = _mm_srli_epi32(d, 24);			// 000000AA
			a = _mm_slli_epi32(rb, 16);			// 00AA0000
			a = _mm_or_si128(rb, a);			// 00AA00AA

			// fix alpha a = a > 127 ? a+1 : a
			// NOTE: If removed an *overflow* will occur with large values (R.N)
			rb = _mm_srli_epi16(a, 7);
			a = _mm_add_epi16(a, rb);
			
			rb = _mm_and_si128(lomask, s);		// 00B00RR		unpack
			rb = _mm_mullo_epi16(rb, a);		// BBRRRR		mul (D[A]*S)
			rb = _mm_srli_epi16(rb, 8);			// 00B00RR		prepack and div [(D[A]*S)]/255

			ag = _mm_srli_epi16(s, 8); 			// 00AA00GG		unpack
			ag = _mm_mullo_epi16(ag, a);		// AAAAGGGG		mul (D[A]*S)
			ag = _mm_andnot_si128(lomask, ag);	// AA00GG00		prepack and div [(D[A]*S)]/255
					
			rb = _mm_or_si128(rb, ag);			// AABGGRR		pack
					
			rb = _mm_sub_epi8(s, rb);			// sub S-[(D[A]*S)/255]
			d = _mm_add_epi8(d, rb);			// add D+[S-(D[A]*S)/255]

			_mm_store_si128(dest128, d);
		}
	}		
}

// TODO: optimize
void PreOver_REF(void* dest, const void* source1, const void* source2, size_t size)
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

		dest8[n+0] = r2 + r1 - (a2*r1)/255;
		dest8[n+1] = g2 + g1 - (a2*g1)/255;
		dest8[n+2] = b2 + b1 - (a2*b1)/255;
		dest8[n+3] = a2 + a1 - (a2*a1)/255;

		// PRECISE
		//if(a2 > 0)
		//{
		//	dest8[n+0] = r2 + r1 - int(float(a2*r1)/255.0f+0.5f);
		//	dest8[n+1] = g2 + g1 - int(float(a2*g1)/255.0f+0.5f);
		//	dest8[n+2] = b2 + b1 - int(float(a2*b1)/255.0f+0.5f);
		//	dest8[n+3] = a2 + a1 - int(float(a2*a1)/255.0f+0.5f);
		//}
		//else
		//{
		//	dest8[n+0] = r1;
		//	dest8[n+1] = g1;
		//	dest8[n+2] = b1;
		//	dest8[n+3] = a1;
		//}
	}
}

} // namespace image
} // namespace utils
} // namespace caspar


