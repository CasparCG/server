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

#include "../StdAfx.h"

#include "frame_producer.h"
#include "frame/basic_frame.h"
#include "frame/frame_transform.h"

#include "color/color_producer.h"
#include "playlist/playlist_producer.h"
#include "separated/separated_producer.h"

#include <common/memory/safe_ptr.h>
#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/utility/move_on_copy.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> g_factories;
	
class destroy_producer_proxy : public frame_producer
{	
	std::unique_ptr<std::shared_ptr<frame_producer>> producer_;
public:
	destroy_producer_proxy(safe_ptr<frame_producer>&& producer) 
		: producer_(new std::shared_ptr<frame_producer>(std::move(producer)))
	{
	}

	~destroy_producer_proxy()
	{		
		static auto destroyers = std::make_shared<tbb::concurrent_bounded_queue<std::shared_ptr<executor>>>();
		static tbb::atomic<int> destroyer_count;

		try
		{
			std::shared_ptr<executor> destroyer;
			if(!destroyers->try_pop(destroyer))
			{
				destroyer.reset(new executor(L"destroyer"));
				destroyer->set_priority_class(below_normal_priority_class);
				if(++destroyer_count > 16)
					CASPAR_LOG(warning) << L"Potential destroyer dead-lock detected.";
				CASPAR_LOG(trace) << "Created destroyer: " << destroyer_count;
			}
				
			auto producer = producer_.release();
			auto pool	  = destroyers;
			destroyer->begin_invoke([=]
			{
				std::unique_ptr<std::shared_ptr<frame_producer>> producer2(producer);

				auto str = (*producer2)->print();
				try
				{
					if(!producer->unique())
						CASPAR_LOG(trace) << str << L" Not destroyed on asynchronous destruction thread: " << producer->use_count();
					else
						CASPAR_LOG(trace) << str << L" Destroying on asynchronous destruction thread.";
				}
				catch(...){}
								
				producer2.reset();
				pool->push(destroyer);
			}); 
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			try
			{
				producer_.reset();
			}
			catch(...){}
		}
	}

	virtual safe_ptr<basic_frame>								receive(int hints) override												{return (*producer_)->receive(hints);}
	virtual safe_ptr<basic_frame>								last_frame() const override		 										{return (*producer_)->last_frame();}
	virtual std::wstring										print() const override													{return (*producer_)->print();}
	virtual boost::property_tree::wptree 						info() const override													{return (*producer_)->info();}
	virtual boost::unique_future<std::wstring>					call(const std::wstring& str) override									{return (*producer_)->call(str);}
	virtual safe_ptr<frame_producer>							get_following_producer() const override									{return (*producer_)->get_following_producer();}
	virtual void												set_leading_producer(const safe_ptr<frame_producer>& producer) override	{(*producer_)->set_leading_producer(producer);}
	virtual uint32_t											nb_frames() const override												{return (*producer_)->nb_frames();}
};

safe_ptr<core::frame_producer> create_producer_destroy_proxy(safe_ptr<core::frame_producer> producer)
{
	return make_safe<destroy_producer_proxy>(std::move(producer));
}

class print_producer_proxy : public frame_producer
{	
	std::shared_ptr<frame_producer> producer_;
public:
	print_producer_proxy(safe_ptr<frame_producer>&& producer) 
		: producer_(std::move(producer))
	{
		CASPAR_LOG(info) << producer_->print() << L" Initialized.";
	}

	~print_producer_proxy()
	{		
		auto str = producer_->print();
		CASPAR_LOG(trace) << str << L" Uninitializing.";
		producer_.reset();
		CASPAR_LOG(info) << str << L" Uninitialized.";
	}

	virtual safe_ptr<basic_frame>								receive(int hints) override												{return (producer_)->receive(hints);}
	virtual safe_ptr<basic_frame>								last_frame() const override		 										{return (producer_)->last_frame();}
	virtual std::wstring										print() const override													{return (producer_)->print();}
	virtual boost::property_tree::wptree 						info() const override													{return (producer_)->info();}
	virtual boost::unique_future<std::wstring>					call(const std::wstring& str) override									{return (producer_)->call(str);}
	virtual safe_ptr<frame_producer>							get_following_producer() const override									{return (producer_)->get_following_producer();}
	virtual void												set_leading_producer(const safe_ptr<frame_producer>& producer) override	{(producer_)->set_leading_producer(producer);}
	virtual uint32_t											nb_frames() const override												{return (producer_)->nb_frames();}
};

safe_ptr<core::frame_producer> create_producer_print_proxy(safe_ptr<core::frame_producer> producer)
{
	return make_safe<print_producer_proxy>(std::move(producer));
}

class last_frame_producer : public frame_producer
{
	const std::wstring			print_;
	const safe_ptr<basic_frame>	frame_;
	const uint32_t				nb_frames_;
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
	virtual uint32_t nb_frames() const {return nb_frames_;}	
	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"last-frame-producer");
		return info;
	}
};

struct empty_frame_producer : public frame_producer
{
	virtual safe_ptr<basic_frame> receive(int){return basic_frame::empty();}
	virtual safe_ptr<basic_frame> last_frame() const{return basic_frame::empty();}
	virtual void set_frame_factory(const safe_ptr<frame_factory>&){}
	virtual uint32_t nb_frames() const {return 0;}
	virtual std::wstring print() const { return L"empty";}
	
	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"empty-producer");
		return info;
	}
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
	
	if(producer == frame_producer::empty())
		producer = create_playlist_producer(my_frame_factory, params);
	
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
		return create_separated_producer(producer, key_producer);
	
	if(producer == frame_producer::empty())
	{
		std::wstring str;
		BOOST_FOREACH(auto& param, params)
			str += param + L" ";
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax.") << arg_value_info(narrow(str)));
	}

	return producer;
}


safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>& factory, const std::wstring& params)
{
	std::wstringstream iss(params);
	std::vector<std::wstring> tokens;
	typedef std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t> > iterator;
	std::copy(iterator(iss),  iterator(), std::back_inserter(tokens));
	return create_producer(factory, tokens);
}

}}