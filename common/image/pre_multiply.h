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
#pragma once

#include "../hardware/cpuid.h"

namespace caspar{ namespace common{ namespace image{
	
void pre_multiply_SSE2	 (void* dest, const void* source, size_t size);
void pre_multiplyParallel_SSE2	 (void* dest, const void* source, size_t size);
void pre_multiply_FastSSE2(void* dest, const void* source, size_t size);
void pre_multiply_REF	 (void* dest, const void* source, size_t size);
void pre_multiplyParallel_REF	 (void* dest, const void* source, size_t size);

typedef void(*pre_multiply_fun)(void*, const void*, size_t);
pre_multiply_fun get_pre_multiply_fun(SIMD simd = REF);

}}}


