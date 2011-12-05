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