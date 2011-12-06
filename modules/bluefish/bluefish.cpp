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

#include "bluefish.h"

#include "consumer/bluefish_consumer.h"

#include "util/blue_velvet.h"

#include <common/log/log.h>
#include <common/utility/string.h>

#include <core/consumer/frame_consumer.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace bluefish {

void init()
{
	try
	{
		blue_initialize();
		core::register_consumer_factory([](const std::vector<std::string>& params)
		{
			return create_consumer(params);
		});
	}
	catch(...){}
}

std::string get_version()
{
	try
	{
		blue_initialize();
	}
	catch(...)
	{
		return "Not found";
	}

	if(!BlueVelvetVersion)
		return "Unknown";

	return std::string(BlueVelvetVersion());
}

std::vector<std::string> get_device_list()
{
	std::vector<std::string> devices;

	try
	{		
		blue_initialize();
		
		auto blue = create_blue();

		for(int n = 1; BLUE_PASS(blue->device_attach(n, FALSE)); ++n)
		{				
			devices.push_back(std::string(get_card_desc(*blue)) + " [" + boost::lexical_cast<std::string>(n) + "]");
			blue->device_detach();		
		}
	}
	catch(...){}

	return devices;
}

}}