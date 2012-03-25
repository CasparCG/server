#pragma once

#include "except.h"

#include <boost/lexical_cast.hpp>

#include <type_traits>
#include <string>

namespace caspar {
		
template<typename T, typename C>
typename std::enable_if<!std::is_convertible<T, std::wstring>::value, typename std::decay<T>::type>::type get_param(const std::wstring& name, C&& params, T fail_value = T())
{	
	auto it = std::find(std::begin(params), std::end(params), name);
	if(it == params.end())	
		return fail_value;
	
	try
	{
		if(++it == params.end())
			throw std::out_of_range("");

		return boost::lexical_cast<std::decay<T>::type>(*it);
	}
	catch(...)
	{		
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Failed to parse param.") << arg_name_info(name) << nested_exception(std::current_exception()));
	}
}

template<typename C>
std::wstring get_param(const std::wstring& name, C&& params, const std::wstring& fail_value = L"")
{	
	auto it = std::find(std::begin(params), std::end(params), name);
	if(it == params.end())	
		return fail_value;
	
	try
	{
		if(++it == params.end())
			throw std::out_of_range("");

		return *it;	
	}
	catch(...)
	{		
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Failed to parse param.") << arg_name_info(name) << nested_exception(std::current_exception()));
	}
}

}