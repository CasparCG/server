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