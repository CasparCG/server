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
 
#ifndef OVER_H_
#define OVER_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
		
void PreOver_SSE2(void* dest, const void* source1, const void* source2, size_t size);
void PreOverParallel_SSE2(void* dest, const void* source1, const void* source2, size_t size);
void PreOver_FastSSE2(void* dest, const void* source1, const void* source2, size_t size);
void PreOver_REF(void* dest, const void* source1, const void* source2, size_t size);
void PreOverParallel_REF(void* dest, const void* source1, const void* source2, size_t size);

typedef void(*PreOverFun)(void*, const void*, const void*, size_t);
PreOverFun GetPreOverFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


