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

#include "copy_field.h"
#include "copy.h"

#include <intrin.h>
#include <functional>

#include "../utility/types.h"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace std::tr1::placeholders; 

namespace caspar { namespace common { namespace image {

void Docopy_fieldParallel(size_t index, const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source, size_t width4)
{
	size_t offset = index*width4;
	size_t size = width4;
	func(reinterpret_cast<s8*>(dest) + offset, reinterpret_cast<const s8*>(source) + offset, size);
}

void copy_fieldParallel(const std::tr1::function<void(void*, const void*, size_t)>& func, void* dest, const void* source, size_t fieldIndex, size_t width, size_t height)
{
	tbb::parallel_for(fieldIndex, height, static_cast<size_t>(2), std::bind(&Docopy_fieldParallel, std::placeholders::_1, func, dest, source, width*4)); // copy for each row
}

copy_field_fun get_copy_field_fun(SIMD /*simd*/)
{
	//if(simd >= SSE2)
	//	return copy_fieldParallel_SSE2;
	//else
	return copy_fieldParallel_REF; // REF is faster
}

void copy_fieldParallel_SSE2(unsigned char* dest, const unsigned char* source, size_t fieldIndex, size_t width, size_t height)
{
	copy_fieldParallel(&copy_SSE2, dest, source, fieldIndex, width, height);
}

void copy_field_REF(unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	for(size_t rowIndex=fieldIndex; rowIndex < height; rowIndex+=2) 
	{
		int offset = width*4*rowIndex;
		__movsd(reinterpret_cast<unsigned long*>(&(pDest[offset])), reinterpret_cast<const unsigned long*>(&(pSrc[offset])), width);
	}
}

void copy_fieldParallel_REF(unsigned char* dest, const unsigned char* source, size_t fieldIndex, size_t width, size_t height)
{
	copy_fieldParallel(&copy_REF, dest, source, fieldIndex, width, height);
}

}}}