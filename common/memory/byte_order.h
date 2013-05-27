/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <intrin.h>

namespace caspar {

template<typename T>
inline T swap_byte_order(T value)
{
	T result;

	swap_byte_order<sizeof(T)>(
			reinterpret_cast<const char*>(&value),
			reinterpret_cast<char*>(&result));

	return result;
}

template<size_t num_bytes>
inline void swap_byte_order(const char* src, char* dest)
{
	for (int i = 0, j = num_bytes - 1; i != num_bytes; ++i, --j)
		dest[i] = src[j];
}

template<>
inline void swap_byte_order<sizeof(unsigned short)>(const char* src, char* dest)
{
	auto result = reinterpret_cast<unsigned short*>(dest);
	auto value = reinterpret_cast<const unsigned short*>(src);
	*result = _byteswap_ushort(*value);
}

template<>
inline void swap_byte_order<sizeof(unsigned long)>(const char* src, char* dest)
{
	auto result = reinterpret_cast<unsigned long*>(dest);
	auto value = reinterpret_cast<const unsigned long*>(src);
	*result = _byteswap_ulong(*value);
}

template<>
inline void swap_byte_order<sizeof(unsigned __int64)>(const char* src, char* dest)
{
	auto result = reinterpret_cast<unsigned __int64*>(dest);
	auto value = reinterpret_cast<const unsigned __int64*>(src);
	*result = _byteswap_uint64(*value);
}

}
