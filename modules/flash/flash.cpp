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

#include "producer/cg_producer.h"
#include "producer/flash_producer.h"

#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>

#include <boost/regex.hpp>

#include <string>
#include <vector>

namespace caspar { namespace flash {

void init()
{
	core::register_producer_factory(create_ct_producer);
	core::register_producer_factory(create_cg_producer);
	core::register_producer_factory(create_swf_producer);
}

std::wstring get_cg_version()
{
	try
	{
		struct dummy_factory : public core::frame_factory
		{
		
			virtual safe_ptr<core::write_frame> create_frame(const void* video_stream_tag, const core::pixel_format_desc& desc) 
			{
				return make_safe<core::write_frame>(nullptr);
			}
	
			virtual core::video_format_desc get_video_format_desc() const
			{
				return core::video_format_desc::get(L"PAL");
			}
		};

		std::vector<std::wstring> params;
		auto producer = make_safe<cg_producer>(flash::create_producer(make_safe<dummy_factory>(), params));

		auto info = producer->template_host_info();
	
		boost::wregex ver_exp(L"version=&quot;(?<VERSION>[^&]*)");
		boost::wsmatch what;
		if(boost::regex_search(info, what, ver_exp))
			return what[L"VERSION"];
	}
	catch(...)
	{

	}

	return L"Unknown";
}

std::wstring get_version()
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
