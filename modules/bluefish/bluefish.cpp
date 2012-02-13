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

#include <boost/lexical_cast.hpp>

namespace caspar { namespace bluefish {

void init()
{
	try
	{
		blue_initialize();
		core::register_consumer_factory([](const std::vector<std::wstring>& params)
		{
			return create_consumer(params);
		});
	}
	catch(...){}
}

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

}}