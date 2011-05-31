#include "..\..\stdafx.h"

#include "Premultiply.hpp"

#include <intrin.h>
#include "../Types.hpp"

namespace caspar{
namespace utils{
namespace image{

PremultiplyFun GetPremultiplyFun(SIMD simd)
{
	if(simd >= SSE2)
		return Premultiply_SSE2;
	else
		return Premultiply_REF;
}

// this function performs precise calculations
void Premultiply_SSE2(void* dest, const void* source, size_t size)
{
	static const size_t stride = sizeof(__m128i)*4;
	static const u32 PSD = 64;

	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);
	static const __m128i amask = _mm_set1_epi32(0xFF000000);
	static const __m128i round = _mm_set1_epi16(128);	

	assert(source != NULL && dest != NULL);
	assert(size % stride == 0);
	
	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	

	__m128i s, rb, ag, a, t;		
	
	for(size_t k = 0, length = size/stride; k != length; ++k)	
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
			_mm_store_si128(dest128, s);
		}		
	}
}

void Premultiply_FastSSE2(void* dest, const void* source, size_t size)
{
	static const size_t stride = sizeof(__m128i)*4;
	static const u32 PSD = 64;

	static const __m128i lomask = _mm_set1_epi32(0x00FF00FF);
	static const __m128i amask = _mm_set1_epi32(0xFF000000);
	

	assert(source != NULL && dest != NULL);
	assert(size % stride == 0);

	const __m128i* source128 = reinterpret_cast<const __m128i*>(source);
	__m128i*	   dest128 = reinterpret_cast<__m128i*>(dest);	

	__m128i s = _mm_setzero_si128();	
	__m128i rb = _mm_setzero_si128();	
	__m128i ag = _mm_setzero_si128();	
	__m128i a = _mm_setzero_si128();

	for(size_t k = 0, length = size/stride; k != length; ++k)	
	{
		// TODO: put prefetch between calculations?(R.N)
		_mm_prefetch(reinterpret_cast<const s8*>(source128 + PSD), _MM_HINT_NTA);

		//work on entire cacheline before next prefetch

		// TODO: assembly optimization use PSHUFD on moves before calculations, lower latency than MOVDQA (R.N) http://software.intel.com/en-us/articles/fast-simd-integer-move-for-the-intel-pentiumr-4-processor/

		for(int n = 0; n < 4; ++n, ++dest128, ++source128)
		{
			s = _mm_load_si128(source128);		// AABBGGRR

			// set alpha to lo16 from source
			rb = _mm_srli_epi32(s, 24);			// 000000AA
			a = _mm_slli_epi32(rb, 16);			// 00AA0000
			a = _mm_or_si128(rb, a);			// 00AA00AA

			// fix alpha a = a > 127 ? a+1 : a
			rb = _mm_srli_epi16(a, 7);
			a = _mm_add_epi16(a, rb);
			
			rb = _mm_and_si128(lomask, s);		// 00BB00RR		unpack
			rb = _mm_mullo_epi16(rb, a);		// BBBBRRRR		mul (D[A]*S)
			rb = _mm_srli_epi16(rb, 8);			// 00BB00RR		prepack and div [(D[A]*S)]/255

			ag = _mm_srli_epi16(s, 8); 			// 00AA00GG		unpack
			ag = _mm_mullo_epi16(ag, a);		// XXXXGGGG		mul (D[A]*S)
			ag = _mm_andnot_si128(lomask, ag);	// XX00GG00		prepack and div [(D[A]*S)]/255
					
			a = _mm_or_si128(rb, ag);			// XXBBGGRR
			a = _mm_andnot_si128(amask, a);		// 00BBGGRR

			s = _mm_and_si128(amask, s);		// AA000000

			s = _mm_or_si128(a, s);				// AABBGGRR		pack

			// TODO: store entire cache line at the same time (write-combining => burst)? are there enough registers? 32 bit mode (special compile for 64bit?) (R.N)
			_mm_store_si128(dest128, s);
		}		
	}
}

void Premultiply_REF(void* dest, const void* source, size_t size)
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

		dest8[n+0] = (r*a)/255;
		dest8[n+1] = (g*a)/255;
		dest8[n+2] = (b*a)/255;
		dest8[n+3] = a;
	}
}

//void StraightTransform_REF(const void* source, void* dest, size_t size)
//{
//	assert(source != NULL && dest != NULL);
//	assert((size % 4) == 0);
//
//	const u8* source8 = reinterpret_cast<const u8*>(source);
//	u8* dest8 = reinterpret_cast<u8*>(dest);
//
//	for(int n = 0; n < size; n+=4)
//	{
//		u32 r = source8[n+0];
//		u32 g = source8[n+1];
//		u32 b = source8[n+2];
//		u32 a = source8[n+3];
//
//		if(a > 0)
//		{
//			dest8[n+0] = (r*255)/a;
//			dest8[n+1] = (g*255)/a;
//			dest8[n+2] = (b*255)/a;
//			dest8[n+3] = a;
//		}
//		else
//		{
//			dest8[n+0] = 0;
//			dest8[n+1] = 0;
//			dest8[n+2] = 0;
//			dest8[n+3] = 0;
//		}
//	}
//}


} // namespace image
} // namespace utils
} // namespace caspar
