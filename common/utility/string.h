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
	   
namespace caspar {

std::wstring widen(const std::string& str);
std::wstring widen(const std::wstring& str);
std::string narrow(const std::wstring& str);	   
std::string narrow(const std::string& str);

template <typename T>
inline T lexical_cast_or_default(const std::wstring str, T fail_value = T())
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