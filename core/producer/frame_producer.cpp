/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "../StdAfx.h"

#include "frame_producer.h"
#include "color/color_producer.h"

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> g_factories;

safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer)
{	
	if(producer == frame_producer::empty())
		return basic_frame::eof();

	auto frame = basic_frame::eof();
	try
	{
		frame = producer->receive();
	}
	catch(...)
	{
		try
		{
			// Producer will be removed since frame == basic_frame::eof.
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << producer->print() << " Failed to receive frame. Removing producer.";
		}
		catch(...){}
	}

	if(frame == basic_frame::eof())
	{
		auto following = producer->get_following_producer();
		following->set_leading_producer(producer);
		producer = std::move(following);		

		return receive_and_follow(producer);
	}
	return frame;
}

void register_producer_factory(const producer_factory_t& factory)
{
	g_factories.push_back(factory);
}

safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	auto producer = frame_producer::empty();
	std::any_of(g_factories.begin(), g_factories.end(), [&](const producer_factory_t& factory) -> bool
		{
			try
			{
				producer = factory(my_frame_factory, params);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return producer != frame_producer::empty();
		});

	if(producer == frame_producer::empty())
		producer = create_color_producer(my_frame_factory, params);

	if(producer == frame_producer::empty())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return producer;
}

}}