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
#include "../utility/types.h"

namespace caspar { namespace common { namespace image {

void shuffle_SSSE3			(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void shuffle_SSE2			(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void shuffle_REF			(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void shuffleParallel_SSSE3	(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void shuffleParallel_SSE2	(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void shuffleParallel_REF	(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);

typedef void(*shuffle_fun)	(void*, const void*, size_t, const u8, const u8, const u8, const u8);
shuffle_fun get_shuffle_fun(SIMD simd = REF);

}}}