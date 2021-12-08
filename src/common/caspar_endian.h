/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace caspar {

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(std::uint8_t), T>::type swap_byte_order(const T& value)
{
    return value;
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(std::uint16_t), T>::type swap_byte_order(const T& value)
{
#ifdef _MSC_VER
    auto swapped = _byteswap_ushort(reinterpret_cast<const unsigned short&>(value));
#elif __GNUC__
    auto swapped = __builtin_bswap16(value);
#endif

    return reinterpret_cast<const T&>(swapped);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(std::uint32_t), T>::type swap_byte_order(const T& value)
{
#ifdef _MSC_VER
    auto swapped = _byteswap_ulong(reinterpret_cast<const unsigned long&>(value));
#elif __GNUC__
    auto swapped = __builtin_bswap32(value);
#endif

    return reinterpret_cast<const T&>(swapped);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(std::uint64_t), T>::type swap_byte_order(const T& value)
{
#ifdef _MSC_VER
    auto swapped = _byteswap_uint64(reinterpret_cast<const unsigned long long&>(value));
#elif __GNUC__
    auto swapped = __builtin_bswap64(value);
#endif

    return reinterpret_cast<const T&>(swapped);
}

} // namespace caspar
