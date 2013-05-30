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
* Author: Robert Nagy, ronag89@gmail.com
*/
#pragma once

#include <boost/lexical_cast.hpp>

#include <type_traits>
#include <string>

namespace caspar {
		
template<typename T, typename C>
typename std::enable_if<!std::is_convertible<T, std::wstring>::value, typename std::decay<T>::type>::type get_param(const std::wstring& name, C&& params, T fail_value = T())
{	
	auto it = std::find(std::begin(params), std::end(params), name);
	if(it == params.end() || ++it == params.end())	
		return fail_value;
	
	T value = fail_value;
	try
	{
		value = boost::lexical_cast<std::decay<T>::type>(*it);
	}
	catch(boost::bad_lexical_cast&){}

	return value;
}

template<typename C>
std::wstring get_param(const std::wstring& name, C&& params, const std::wstring& fail_value = L"")
{	
	auto it = std::find(std::begin(params), std::end(params), name);
	if(it == params.end() || ++it == params.end())	
		return fail_value;
	return *it;
}

}