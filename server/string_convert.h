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
 
#ifndef _STRING_CONVERT_H_
#define _STRING_CONVERT_H_

#include <locale>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
       
namespace caspar { namespace utils {

std::wstring widen(const std::string& str, const std::locale& locale = std::locale())
{
	std::wstringstream wsstr ;
	wsstr.imbue(locale);
	const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wsstr.getloc()) ;
	for(size_t i = 0 ;i < str.size(); ++i)
		wsstr << ctfacet.widen(str[i]) ;
	return wsstr.str() ;
}
       
std::string narrow(const std::wstring& str, const std::locale& locale = std::locale())
{
	std::stringstream sstr;
	sstr.imbue(locale);
	const std::ctype<char>& ctfacet = std::use_facet<std::ctype<char>>(sstr.getloc());
	for(size_t i = 0; i < str.size(); ++i)
		sstr << ctfacet.narrow(str[i], 0) ;
	return sstr.str() ;
}

std::string narrow_to_latin1(const std::wstring& wideString)
{
	std::string destBuffer;
	//28591 = ISO 8859-1 Latin I
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, 0, 0, nullptr, nullptr);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.resize(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, &destBuffer[0], destBuffer.size(), nullptr, nullptr);
	}
	destBuffer.resize(bytesWritten);
	return destBuffer;
}

template <typename T>
T lexical_cast_or_default(const std::wstring str, T defaultValue)
{
	try
	{
		if(!str.empty())
			return boost::lexical_cast<T>(str);
	}
	catch(...){}
	return defaultValue;
}

}}

#endif