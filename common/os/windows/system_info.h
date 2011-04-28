#pragma once

#include <windows.h>

#include <string>
#include <sstream>

namespace caspar {
	
static std::wstring get_cpu_info()
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

static std::wstring get_system_product_name()
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