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
 
#include "..\..\stdafx.h"

#include "CopyField.hpp"
#include "Copy.hpp"

#include <intrin.h>
#include <functional>

#include "../Types.hpp"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace std::tr1::placeholders; 

namespace caspar{
namespace utils{
namespace image{

void DoCopyFieldParallel(size_t index, const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source, size_t width4)
{
	size_t offset = index*width4;
	size_t size = width4;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source) + offset, size);
}

void CopyFieldParallel(const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source, size_t fieldIndex, size_t width, size_t height)
{
	tbb::parallel_for(fieldIndex, height, static_cast<size_t>(2), std::tr1::bind(&DoCopyFieldParallel, _1, func, dest, source, width*4)); // copy for each row
}

CopyFieldFun GetCopyFieldFun(SIMD simd)
{
	//if(simd >= SSE2)
	//	return CopyFieldParallel_SSE2;
	//else
	return CopyFieldParallel_REF; // REF is faster
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
	_mm_mfence();	//ensure last WC buffers get flushed to memory
}

void CopyFieldParallel_SSE2(unsigned char* dest, unsigned char* source, size_t fieldIndex, size_t width, size_t height)
{
	CopyFieldParallel(&Copy_SSE2, dest, source, fieldIndex, width, height);
}

void CopyField_REF(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	for(int rowIndex=fieldIndex; rowIndex < height; rowIndex+=2) 
	{
		int offset = width*4*rowIndex;
		__movsd(reinterpret_cast<unsigned long*>(&(pDest[offset])), reinterpret_cast<const unsigned long*>(&(pSrc[offset])), width);
	}
}

void CopyFieldParallel_REF(unsigned char* dest, unsigned char* source, size_t fieldIndex, size_t width, size_t height)
{
	CopyFieldParallel(&Copy_REF, dest, source, fieldIndex, width, height);
}

}
}
}