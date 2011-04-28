#pragma once


#include <windows.h>

#include <string>

namespace caspar {

static std::wstring get_win_product_name()
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

static std::wstring get_win_sp_version()
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