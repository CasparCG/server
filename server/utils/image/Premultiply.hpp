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
 
#ifndef PREMULTIPLY_H_
#define PREMULTIPLY_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
	
void Premultiply_SSE2	 (void* dest, const void* source, size_t size);
void PremultiplyParallel_SSE2	 (void* dest, const void* source, size_t size);
void Premultiply_FastSSE2(void* dest, const void* source, size_t size);
void Premultiply_REF	 (void* dest, const void* source, size_t size);
void PremultiplyParallel_REF	 (void* dest, const void* source, size_t size);

typedef void(*PremultiplyFun)(void*, const void*, size_t);
PremultiplyFun GetPremultiplyFun(SIMD simd = REF);


//void StraightTransform_SSE2(const void* source, void* dest, size_t size);
//void StraightTransform_REF(const void* source, void* dest, size_t size);
//
//typedef void(*StraightTransformFun)(const void*, void*, size_t);
//StraightTransformFun GetStraightTransformFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


