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

#include "StdAfx.h"

#include "flash.h"

#include "producer/cg_proxy.h"
#include "producer/flash_producer.h"

#include <common/env.h>
#include <common/os/windows/windows.h>

#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/system_info_provider.h>

#include <boost/property_tree/ptree.hpp>

#include <string>

namespace caspar { namespace flash {

std::wstring version();
std::wstring cg_version();

void init(
		const spl::shared_ptr<core::media_info_repository>& media_info_repo,
		const spl::shared_ptr<core::system_info_provider_repository>& info_provider_repo)
{
	core::register_producer_factory(create_ct_producer);
	core::register_producer_factory(create_swf_producer);
	media_info_repo->register_extractor([](const std::wstring& file, const std::wstring& extension, core::media_info& info)
	{
		if (extension != L".CT" && extension != L".SWF")
			return false;

		info.clip_type = L"MOVIE";

		return true;
	});
	info_provider_repo->register_system_info_provider([](boost::property_tree::wptree& info)
	{
		info.add(L"system.flash", version());
	});
	info_provider_repo->register_version_provider(L"FLASH", &version);
	info_provider_repo->register_version_provider(L"TEMPLATEHOST", &cg_version);
}

std::wstring cg_version()
{
	return L"Unknown";
}

std::wstring version()
{		
	std::wstring version = L"Not found";
#ifdef WIN32 
	HKEY   hkey;
 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Macromedia\\FlashPlayerActiveX"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t ver_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(ver_str);
		RegQueryValueEx(hkey, TEXT("Version"), NULL, &dwType, (PBYTE)&ver_str, &dwSize);
 
		version = ver_str;

		RegCloseKey(hkey);
	}
#endif
	return version;
}

}}
