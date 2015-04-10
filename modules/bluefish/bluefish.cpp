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

#include "bluefish.h"

#include "consumer/bluefish_consumer.h"

#include "util/blue_velvet.h"

#include <common/log.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/system_info_provider.h>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace bluefish {

std::wstring version()
{
	try
	{
		blue_initialize();
	}
	catch(...)
	{
		return L"Not found";
	}

	if(!BlueVelvetVersion)
		return L"Unknown";

	return u16(BlueVelvetVersion());
}

std::vector<std::wstring> device_list()
{
	std::vector<std::wstring> devices;

	try
	{		
		blue_initialize();
		
		auto blue = create_blue();

		for(int n = 1; BLUE_PASS(blue->device_attach(n, FALSE)); ++n)
		{				
			devices.push_back(std::wstring(get_card_desc(*blue)) + L" [" + boost::lexical_cast<std::wstring>(n) + L"]");
			blue->device_detach();		
		}
	}
	catch(...){}

	return devices;
}

void init(core::module_dependencies dependencies)
{
	try
	{
		blue_initialize();
	}
	catch(...){}

	core::register_consumer_factory(create_consumer);
	core::register_preconfigured_consumer_factory(L"bluefish", create_preconfigured_consumer);
	dependencies.system_info_provider_repo->register_system_info_provider([](boost::property_tree::wptree& info)
	{
		info.add(L"system.bluefish.version", version());

		for (auto device : device_list())
			info.add(L"system.bluefish.device", device);
	});
}

}}