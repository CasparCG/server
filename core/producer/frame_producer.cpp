#include "../StdAfx.h"

#include "frame_producer.h"
#include "color/color_producer.h"

#include <common/memory/safe_ptr.h>

#include <tbb/spin_rw_mutex.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> p_factories;
tbb::spin_rw_mutex p_factories_mutex;

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

std::wostream& operator<<(std::wostream& out, const frame_producer& producer)
{
	out << producer.print().c_str();
	return out;
}

std::wostream& operator<<(std::wostream& out, const safe_ptr<const frame_producer>& producer)
{
	out << producer->print().c_str();
	return out;
}

void register_producer_factory(const producer_factory_t& factory)
{
	tbb::spin_rw_mutex::scoped_lock(p_factories_mutex, true);
	p_factories.push_back(factory);
}

safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	tbb::spin_rw_mutex::scoped_lock(p_factories_mutex, false);
	auto producer = frame_producer::empty();
	std::any_of(p_factories.begin(), p_factories.end(), [&](const producer_factory_t& factory) -> bool
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