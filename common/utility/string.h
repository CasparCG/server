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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
	   
namespace caspar {

std::wstring u16(const std::string& str);
std::wstring u8_u16(const std::string& str);
std::wstring u16(const std::wstring& str);

std::string u16_u8(const std::wstring& str);
std::string u8(const std::wstring& str);
std::string u8(const std::string& str);	

namespace detail
{
	template<typename T>
	struct convert_utf_string
	{
		std::wstring operator()(const std::wstring& str)
		{
			return u16(str);
		}
	};

	template<>
	struct convert_utf_string<std::string>
	{
		std::string operator()(const std::wstring& str)
		{
			return u8(str);
		}
	};
};

template<typename T1, typename T2>
bool iequals(const T1& lhs, const T2& rhs)
{
	return boost::iequals(u16(lhs), u16(rhs));
}

template<typename T>
T to_upper_copy(const T& str)
{
	return detail::convert_utf_string<T>()(boost::to_upper_copy(u16(str)));
}

template<typename T>
T to_lower_copy(const T& str)
{
	return detail::convert_utf_string<T>()(boost::to_lower_copy(u16(str)));
}

template<typename T>
void to_upper(T& str)
{
	str = detail::convert_utf_string<T>()(boost::to_upper_copy(u16(str)));
}

template<typename T>
void to_lower(T& str)
{
	str = detail::convert_utf_string<T>()(boost::to_lower_copy(u16(str)));
}

template <typename T>
inline T lexical_cast_or_default(const std::wstring& str, T fail_value = T())
{
	try
	{
		return boost::lexical_cast<T>(str);
	}
	catch(boost::bad_lexical_cast&)
	{
		return fail_value;
	}
}

template <typename T>
inline T lexical_cast_or_default(const std::string& str, T fail_value = T())
{
	try
	{
		return boost::lexical_cast<T>(str);
	}
	catch(boost::bad_lexical_cast&)
	{
		return fail_value;
	}
}

}