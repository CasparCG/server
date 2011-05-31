#include "..\..\stdafx.h"

#include "Clear.hpp"

#include <intrin.h>
#include "../Types.hpp"

namespace caspar{
namespace utils{
namespace image{

ClearFun GetClearFun(SIMD simd)
{
	if(simd >= SSE2)
		return Clear_SSE2;
	else
		return Clear_REF;
}

// TODO: (R.N) optimize => prefetch and cacheline loop unroll
void Clear_SSE2(void* dest, size_t size)
{
	__m128i val = _mm_setzero_si128();
	__m128i* ptr = reinterpret_cast<__m128i*>(dest);

	int times = size / 16;
	for(int i=0; i < times; ++i) {
		_mm_stream_si128(ptr, val);
		ptr++;
	}
}

void Clear_REF(void* dest, size_t size)
{
	__stosd(reinterpret_cast<unsigned long*>(dest), 0, size/4);
}

}
}
}