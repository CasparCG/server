#pragma once

#include <locale>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
	   
namespace caspar {

inline std::wstring widen(const std::string& str, const std::locale& locale = std::locale())
{
	std::wstringstream wsstr ;
	wsstr.imbue(locale);
	const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wsstr.getloc()) ;
	for(size_t i = 0 ;i < str.size(); ++i)
		wsstr << ctfacet.widen(str[i]) ;
	return wsstr.str() ;
}

inline std::wstring widen(const std::wstring& str, const std::locale&)
{
	return str;
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
	   
inline std::string narrow(const std::wstring& str, const std::locale& locale = std::locale())
{
	std::stringstream sstr;
	sstr.imbue(locale);
	const std::ctype<char>& ctfacet = std::use_facet<std::ctype<char>>(sstr.getloc());
	for(size_t i = 0; i < str.size(); ++i)
		sstr << ctfacet.narrow(str[i], 0) ;
	return sstr.str() ;
}
	   
inline std::string narrow(const std::string& str, const std::locale&)
{
	return str ;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
//
//inline std::string narrow_to_latin1(const std::wstring& wideString)
//{
//	std::string destBuffer;
//	//28591 = ISO 8859-1 Latin I
//	int bytesWritten = 0;
//	int multibyteBufferCapacity = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, 0, 0, 
//													   nullptr, nullptr);
//	if(multibyteBufferCapacity > 0) 
//	{
//		destBuffer.resize(multibyteBufferCapacity);
//		bytesWritten = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, &destBuffer[0], 
//											destBuffer.size(), nullptr, nullptr);
//	}
//	destBuffer.resize(bytesWritten);
//	return destBuffer;
//}

template <typename T>
inline T lexical_cast_or_default(const std::wstring str, T defaultValue = T())
{
	try
	{
		if(!str.empty())
			return boost::lexical_cast<T>(str);
	}
	catch(...){}
	return defaultValue;
}

}