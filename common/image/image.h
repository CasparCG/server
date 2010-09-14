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

#include "../utility/types.h"
#include "../hardware/cpuid.h"

namespace caspar{ namespace common{ namespace image{	
	
void set_version(SIMD simd = AUTO);

void shuffle(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void pre_over(void* dest, const void* source1, const void* source2, size_t size);
void lerp(void* dest, const void* source1, const void* source2, float alpha, size_t size);
void pre_multiply(void* dest, const void* source, size_t size);
void copy(void* dest, const void* source, size_t size);
void clear(void* dest, size_t size);
void copy_field(unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);

namespace detail
{
	typedef void(*pre_over_fun)(void*, const void*, const void*, size_t);
	typedef void(*shuffle_fun)(void*, const void*, size_t, const u8, const u8, const u8, const u8);
	typedef void(*lerp_fun)(void*, const void*, const void*, float, size_t);
	typedef void(*pre_multiply_fun)(void*, const void*, size_t);
	typedef void(*copy_fun)(void*, const void*, size_t);
	typedef void(*copy_field_fun)(unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
	typedef void(*clear_fun)(void*, size_t);
	
	extern pre_over_fun		pre_over;
	extern shuffle_fun		shuffle;
	extern lerp_fun			lerp;
	extern pre_multiply_fun	pre_multiply;
	extern copy_fun			copy;
	extern copy_field_fun	copy_field;
	extern clear_fun		clear;
}

}}}