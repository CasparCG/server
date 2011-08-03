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
#include "frame/basic_frame.h"
#include "frame/audio_transform.h"

#include "color/color_producer.h"
#include "separated/separated_producer.h"

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> g_factories;

frame_producer::frame_producer() : last_frame_(core::basic_frame::empty()){}

const safe_ptr<frame_producer>& frame_producer::empty() // nothrow
{
	struct empty_frame_producer : public frame_producer
	{
		virtual safe_ptr<basic_frame> receive(){return basic_frame::empty();}
		virtual void set_frame_factory(const safe_ptr<frame_factory>&){}
		virtual std::wstring print() const { return L"empty";}
	};
	static safe_ptr<frame_producer> producer = make_safe<empty_frame_producer>();
	return producer;
}	

safe_ptr<basic_frame> frame_producer::receive_save_last()
{
	auto frame = receive();
	if(frame != core::basic_frame::late())
	{
		last_frame_ = make_safe<basic_frame>(frame);
		last_frame_->get_audio_transform().set_has_audio(false);
	}	
	return frame;
}

safe_ptr<basic_frame> receive(const safe_ptr<frame_producer>& producer)
{
	return producer->receive_save_last();
}

safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer)
{	
	if(producer == frame_producer::empty())
		return basic_frame::eof();

	auto frame = basic_frame::eof();
	try
	{
		frame = receive(producer);
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
		CASPAR_LOG(info) << producer->print() << " End Of File.";
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

safe_ptr<core::frame_producer> do_create_producer(const safe_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
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


safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
{	
	auto producer = do_create_producer(my_frame_factory, params);
	auto key_producer = frame_producer::empty();
	
	try // to find a key file.
	{
		auto params_copy = params;
		if(params_copy.size() > 0)
		{
			params_copy[0] += L"_A";
			key_producer = do_create_producer(my_frame_factory, params_copy);			
		}
	}
	catch(...){}

	if(key_producer != frame_producer::empty())
		return create_separated_producer(producer, key_producer);

	return producer;
}

}}