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

#include "../stdafx.h"

#include <boost/locale.hpp>

#include "utf8conv.h"

namespace caspar {
	
std::wstring widen(const std::string& str)
{
	return utf8util::UTF16FromUTF8(str);//std::wstring(str.begin(), str.end());
}

std::wstring widen(const std::wstring& str)
{
	return str;
}
	   
std::string narrow(const std::wstring& str)
{
	return utf8util::UTF8FromUTF16(str);//std::string(str.begin(), str.end());
}
	   
std::string narrow(const std::string& str)
{
	return str ;
}

}