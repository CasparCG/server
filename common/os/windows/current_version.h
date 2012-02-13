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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "windows.h"

#include <string>

namespace caspar {

static std::wstring win_product_name()
{
	std::wstring result = L"Unknown Windows Product Name.";
	HKEY hkey; 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t p_name_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(p_name_str);

		if(RegQueryValueEx(hkey, TEXT("ProductName"), NULL, &dwType, (PBYTE)&p_name_str, &dwSize) == ERROR_SUCCESS)		
			result = p_name_str;		
		 
		RegCloseKey(hkey);
	}
	return result;
}

static std::wstring win_sp_version()
{
	std::wstring result =  L"";
	HKEY hkey; 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t csd_ver_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(csd_ver_str);

		if(RegQueryValueEx(hkey, TEXT("CSDVersion"), NULL, &dwType, (PBYTE)&csd_ver_str, &dwSize) == ERROR_SUCCESS)		
			result = csd_ver_str;
		 
		RegCloseKey(hkey);
	}
	return result;
}

}