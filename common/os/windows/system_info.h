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

#include "windows.h"

#include <string>
#include <sstream>

namespace caspar {
	
static std::wstring cpu_info()
{
	std::wstring cpu_name = L"Unknown CPU";
	HKEY hkey; 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t p_name_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(p_name_str);

		if(RegQueryValueEx(hkey, TEXT("ProcessorNameString"), NULL, &dwType, (PBYTE)&p_name_str, &dwSize) == ERROR_SUCCESS)		
			cpu_name = p_name_str;		
		 
		RegCloseKey(hkey);
	}


	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	std::wstringstream s;

	s << cpu_name << L" Physical Threads: " << sysinfo.dwNumberOfProcessors;

	return s.str();
}

static std::wstring system_product_name()
{
	std::wstring system_product_name = L"Unknown System";
	HKEY hkey; 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\BIOS"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t p_name_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(p_name_str);

		if(RegQueryValueEx(hkey, TEXT("SystemProductName"), NULL, &dwType, (PBYTE)&p_name_str, &dwSize) == ERROR_SUCCESS)		
			system_product_name = p_name_str;		
		 
		RegCloseKey(hkey);
	}

	return system_product_name;
}


}