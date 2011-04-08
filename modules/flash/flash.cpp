#include "StdAfx.h"

#include "flash.h"

#include "producer/cg_producer.h"
#include "producer/flash_producer.h"
#include "producer/flash_producer.h"

#include <common/env.h>

namespace caspar{

void init_flash()
{
	core::register_producer_factory(create_ct_producer);
}

std::wstring get_cg_version()
{
	return L"Unknown";
}

std::wstring g_version = L"Not found";
void setup_version()
{ 
#ifdef WIN32
	HKEY   hkey;
 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Macromedia\\FlashPlayerActiveX"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t ver_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(ver_str);
		RegQueryValueEx(hkey, TEXT("Version"), NULL, &dwType, (PBYTE)&ver_str, &dwSize);
 
		g_version = ver_str;

		RegCloseKey(hkey);
	}
#endif
}

std::wstring get_flash_version()
{		
	boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(setup_version, flag);

	return g_version;
}

}
