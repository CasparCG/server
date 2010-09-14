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
	
void copy_SSE2	 (void* dest, const void* source, size_t size);
void copy_REF	 (void* dest, const void* source, size_t size);

void copyParallel_SSE2	 (void* dest, const void* source, size_t size);
void copyParallel_REF	 (void* dest, const void* source, size_t size);

typedef void(*copy_fun)(void*, const void*, size_t);
copy_fun get_copy_fun(SIMD simd = REF);

}}} 


