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

#include "image.h"

#include "copy.h"
#include "copy_field.h"
#include "clear.h"
//#include "Transition.hpp"

namespace caspar{ namespace common{ namespace image{

namespace detail
{
	cpuid my_cpuid;
	copy_fun			copy = get_copy_fun(my_cpuid.SIMD);
	copy_field_fun		copy_field = get_copy_field_fun(my_cpuid.SIMD);
	clear_fun			clear = get_clear_fun(my_cpuid.SIMD);
}

void copy(void* dest, const void* source, size_t size)
{
	(*detail::copy)(dest, source, size);
}

void copy_field(unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	(*detail::copy_field)(pDest, pSrc, fieldIndex, width, height);
}

void clear(void* dest, size_t size)
{
	(*detail::clear)(dest, size);
}

}}}