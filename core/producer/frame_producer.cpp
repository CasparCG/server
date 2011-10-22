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
#include "frame/frame_transform.h"

#include "color/color_producer.h"
#include "separated/separated_producer.h"

#include <common/memory/safe_ptr.h>
#include <common/exception/exceptions.h>

#include <concrt_extras.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> g_factories;
	
struct destruction_context
{
	std::shared_ptr<frame_producer> producer;
	Concurrency::event				event;

	destruction_context(std::shared_ptr<frame_producer>&& producer) 
		: producer(producer)
	{
	}
};

void __cdecl destroy_producer(LPVOID lpParam)
{
	auto destruction = std::unique_ptr<destruction_context>(static_cast<destruction_context*>(lpParam));
	
	try
	{		
		if(destruction->producer.unique())
		{
			Concurrency::scoped_oversubcription_token oversubscribe;
			destruction->producer.reset();
		}
		else
			CASPAR_LOG(warning) << destruction->producer->print() << " Not destroyed asynchronously.";		
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}
	
	destruction->event.set();
}

void __cdecl destroy_and_wait_producer(LPVOID lpParam)
{
	try
	{
		auto destruction = static_cast<destruction_context*>(lpParam);
		Concurrency::CurrentScheduler::ScheduleTask(destroy_producer, lpParam);
		if(destruction->event.wait(1000) == Concurrency::COOPERATIVE_WAIT_TIMEOUT)
			CASPAR_LOG(warning) << " Potential destruction deadlock detected. Might leak resources.";
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}
}

class destroy_producer_proxy : public frame_producer
{
	std::shared_ptr<frame_producer> producer_;
public:
	destroy_producer_proxy(const std::shared_ptr<frame_producer>& producer) 
		: producer_(producer)
	{
	}

	~destroy_producer_proxy()
	{		
		Concurrency::CurrentScheduler::ScheduleTask(destroy_producer, new destruction_context(std::move(producer_)));
	}

	virtual safe_ptr<basic_frame>		receive(int hints)												{return producer_->receive(hints);}
	virtual safe_ptr<basic_frame>		last_frame() const												{return producer_->last_frame();}
	virtual std::wstring				print() const													{return producer_->print();}
	virtual void						param(const std::wstring& str)									{producer_->param(str);}
	virtual safe_ptr<frame_producer>	get_following_producer() const									{return producer_->get_following_producer();}
	virtual void						set_leading_producer(const safe_ptr<frame_producer>& producer)	{producer_->set_leading_producer(producer);}
	virtual int64_t						nb_frames() const												{return producer_->nb_frames();}
};

safe_ptr<core::frame_producer> create_destroy_producer_proxy(const safe_ptr<frame_producer>& producer)
{
	return make_safe<destroy_producer_proxy>(producer);
}

class last_frame_producer : public frame_producer
{
	const std::wstring			print_;
	const safe_ptr<basic_frame>	frame_;
	const int64_t				nb_frames_;
public:
	last_frame_producer(const safe_ptr<frame_producer>& producer) 
		: print_(producer->print())
		, frame_(producer->last_frame() != basic_frame::eof() ? producer->last_frame() : basic_frame::empty())
		, nb_frames_(producer->nb_frames())
	{
	}
	
	virtual safe_ptr<basic_frame> receive(int){return frame_;}
	virtual safe_ptr<core::basic_frame> last_frame() const{return frame_;}
	virtual std::wstring print() const{return L"dummy[" + print_ + L"]";}
	virtual int64_t nb_frames() const {return nb_frames_;}	
};

struct empty_frame_producer : public frame_producer
{
	virtual safe_ptr<basic_frame> receive(int){return basic_frame::empty();}
	virtual safe_ptr<basic_frame> last_frame() const{return basic_frame::empty();}
	virtual void set_frame_factory(const safe_ptr<frame_factory>&){}
	virtual int64_t nb_frames() const {return 0;}
	virtual std::wstring print() const { return L"empty";}
};

const safe_ptr<frame_producer>& frame_producer::empty() // nothrow
{
	static safe_ptr<frame_producer> producer = make_safe<empty_frame_producer>();
	return producer;
}	

safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer, int hints)
{	
	auto frame = producer->receive(hints);
	if(frame == basic_frame::eof())
	{
		CASPAR_LOG(info) << producer->print() << " End Of File.";
		auto following = producer->get_following_producer();
		if(following != frame_producer::empty())
		{
			following->set_leading_producer(producer);
			producer = std::move(following);
		}
		else
			producer = make_safe<last_frame_producer>(producer);

		return receive_and_follow(producer, hints);
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
			if(key_producer == frame_producer::empty())
			{
				params_copy[0] += L"LPHA";
				key_producer = do_create_producer(my_frame_factory, params_copy);	
			}
		}
	}
	catch(...){}

	if(producer != frame_producer::empty() && key_producer != frame_producer::empty())
		producer = create_separated_producer(producer, key_producer);
	
	if(producer == frame_producer::empty())
	{
		std::wstring str;
		BOOST_FOREACH(auto& param, params)
			str += param + L" ";
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax.") << arg_value_info(narrow(str)));
	}

	return producer;
}

}}