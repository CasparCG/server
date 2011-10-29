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

#include "frame_consumer.h"

#include <common/env.h>
#include <common/memory/safe_ptr.h>
#include <common/exception/exceptions.h>

#include <concurrent_vector.h>

namespace caspar { namespace core {
	
size_t consumer_buffer_depth()
{
	return env::properties().get("configuration.consumers.buffer-depth", 5);
}

struct destruction_context
{
	std::shared_ptr<frame_consumer> consumer;
	Concurrency::event				event;

	destruction_context(std::shared_ptr<frame_consumer>&& consumer) 
		: consumer(consumer)
	{
	}
};

void __cdecl destroy_consumer(LPVOID lpParam)
{
	auto destruction = std::unique_ptr<destruction_context>(static_cast<destruction_context*>(lpParam));
	
	try
	{		
		if(destruction->consumer.unique())
		{
			Concurrency::scoped_oversubcription_token oversubscribe;
			destruction->consumer.reset();
		}
		else
			CASPAR_LOG(warning) << destruction->consumer->print() << " Not destroyed asynchronously.";		
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}
	
	destruction->event.set();
}

void __cdecl destroy_and_wait_consumer(LPVOID lpParam)
{
	try
	{
		auto destruction = static_cast<destruction_context*>(lpParam);
		Concurrency::CurrentScheduler::ScheduleTask(destroy_consumer, lpParam);
		if(destruction->event.wait(1000) == Concurrency::COOPERATIVE_WAIT_TIMEOUT)
			CASPAR_LOG(warning) << " Potential destruction deadlock detected. Might leak resources.";
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}
}
	
class destroy_consumer_proxy : public frame_consumer
{
	std::shared_ptr<frame_consumer> consumer_;
public:
	destroy_consumer_proxy(const std::shared_ptr<frame_consumer>& consumer) 
		: consumer_(consumer)
	{
	}

	~destroy_consumer_proxy()
	{		
		Concurrency::CurrentScheduler::ScheduleTask(destroy_consumer, new destruction_context(std::move(consumer_)));
	}	
	
	virtual bool send(const safe_ptr<read_frame>& frame)					{return consumer_->send(frame);}
	virtual void initialize(const video_format_desc& format_desc)			{return consumer_->initialize(format_desc);}
	virtual std::wstring print() const										{return consumer_->print();}
	virtual bool has_synchronization_clock() const							{return consumer_->has_synchronization_clock();}
	virtual const core::video_format_desc& get_video_format_desc() const	{return consumer_->get_video_format_desc();}
	virtual size_t buffer_depth() const										{return consumer_->buffer_depth();}
};

Concurrency::concurrent_vector<std::shared_ptr<consumer_factory_t>> g_factories;

void register_consumer_factory(const consumer_factory_t& factory)
{
	g_factories.push_back(std::make_shared<consumer_factory_t>(factory));
}

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	auto consumer = frame_consumer::empty();
	std::any_of(g_factories.begin(), g_factories.end(), [&](const std::shared_ptr<consumer_factory_t>& factory) -> bool
		{
			try
			{
				consumer = (*factory)(params);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return consumer != frame_consumer::empty();
		});

	if(consumer == frame_consumer::empty())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return make_safe<destroy_consumer_proxy>(consumer);
}

}}