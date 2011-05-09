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

#include "Clear.hpp"

#include <intrin.h>
#include <functional>

#include "../Types.hpp"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace std::tr1::placeholders;

namespace caspar{
namespace utils{
namespace image{

static const size_t STRIDE = sizeof(__m128i)*4;

void DoClearParallel(const tbb::blocked_range<size_t>& r, const std::tr1::function<void(void*, size_t)>& func, void* dest)
{
	size_t offset = r.begin()*STRIDE;
	size_t size = r.size()*STRIDE;
	func(reinterpret_cast<s8*>(dest) + offset, size);
}

void ClearParallel(const std::tr1::function<void(void*, size_t)>& func, void* dest, size_t size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/STRIDE), std::tr1::bind(&DoClearParallel, _1, func, dest));	
}

ClearFun GetClearFun(SIMD simd)
{
	if(simd >= SSE2)
		return ClearParallel_SSE2;
	else
		return ClearParallel_REF;
}

// TODO: (R.N) optimize => prefetch and cacheline loop unroll
void Clear_SSE2(void* dest, size_t size)
{
	__m128i val = _mm_setzero_si128();
	__m128i* ptr = reinterpret_cast<__m128i*>(dest);

	int times = size / 16;
	for(int i=0; i < times; ++i) 
	{
		_mm_stream_si128(ptr, val);
		ptr++;
	}
}

void ClearParallel_SSE2(void* dest, size_t size)
{
	ClearParallel(&Clear_SSE2, dest, size);
}

void Clear_REF(void* dest, size_t size)
{
	__stosd(reinterpret_cast<unsigned long*>(dest), 0, size/4);
}

void ClearParallel_REF(void* dest, size_t size)
{
	ClearParallel(&Clear_REF, dest, size);
}

}
}
}