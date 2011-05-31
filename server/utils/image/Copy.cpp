#include "..\..\stdafx.h"

#include "Copy.hpp"

#include <intrin.h>
#include "../Types.hpp"

namespace caspar{
namespace utils{
namespace image{

CopyFun GetCopyFun(SIMD simd)
{
	if(simd >= SSE2)
		return Copy_SSE2;
	else
		return Copy_REF;
}

// TODO: (R.N) optimize => prefetch and cacheline loop unroll
void Copy_SSE2(void* dest, const void* source, size_t size)
{
	__m128i val = _mm_setzero_si128();
	__m128i* pD = reinterpret_cast<__m128i*>(dest);
	const __m128i* pS = reinterpret_cast<const __m128i*>(source);

	int times = size / 16;
	for(int i=0; i < times; ++i) 
	{
		val = _mm_load_si128(pS);
		_mm_stream_si128(pD, val);

		++pD;
		++pS;
	}
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void Copy_REF(void* dest, const void* source, size_t size)
{
	__movsd(reinterpret_cast<unsigned long*>(dest), reinterpret_cast<const unsigned long*>(source), size/4);
}

CopyFieldFun GetCopyFieldFun(SIMD simd)
{
	if(simd >= SSE2)
		return CopyField_SSE2;
	else
		return CopyField_REF;
}

// TODO: (R.N) optimize => prefetch and cacheline loop unroll
void CopyField_SSE2(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	for(int rowIndex=fieldIndex; rowIndex < height; rowIndex+=2) 
	{
		int offset = width*4*rowIndex;

		__m128i val = _mm_setzero_si128();
		__m128i* pD = reinterpret_cast<__m128i*>(&(pDest[offset]));
		const __m128i* pS = reinterpret_cast<const __m128i*>(&(pSrc[offset]));

		int times = width / 4;
		for(int i=0; i < times; ++i) 
		{
			val = _mm_load_si128(pS);
			_mm_stream_si128(pD, val);

			++pD;
			++pS;
		}
	}
}

void CopyField_REF(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	for(int rowIndex=fieldIndex; rowIndex < height; rowIndex+=2) 
	{
		int offset = width*4*rowIndex;
		__movsd(reinterpret_cast<unsigned long*>(&(pDest[offset])), reinterpret_cast<const unsigned long*>(&(pSrc[offset])), width);
	}
}

}
}
}