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

#include "../frame/draw_frame.h"
#include "../frame/frame_transform.h"

#include "color/color_producer.h"
#include "separated/separated_producer.h"

#include <common/assert.h>
#include <common/except.h>
#include <common/concurrency/executor.h>
#include <common/concurrency/async.h>
#include <common/spl/memory.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> g_factories;

void register_producer_factory(const producer_factory_t& factory)
{
	g_factories.push_back(factory);
}

boost::unique_future<std::wstring> frame_producer::call(const std::wstring&) 
{
	BOOST_THROW_EXCEPTION(not_supported());
}

struct empty_frame_producer : public frame_producer
{
	virtual spl::shared_ptr<draw_frame> receive(int){return draw_frame::empty();}
	virtual spl::shared_ptr<draw_frame> last_frame() const{return draw_frame::empty();}
	virtual void set_frame_factory(const spl::shared_ptr<frame_factory>&){}
	virtual uint32_t nb_frames() const {return 0;}
	virtual std::wstring print() const { return L"empty";}
	
	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"empty-producer");
		return info;
	}
};

const spl::shared_ptr<frame_producer>& frame_producer::empty() // nothrow
{
	static spl::shared_ptr<frame_producer> producer = spl::make_shared<empty_frame_producer>();
	return producer;
}	

class producer_proxy_base : public frame_producer
{	
protected:
	std::shared_ptr<frame_producer> producer_;
public:
	producer_proxy_base(spl::shared_ptr<frame_producer>&& producer) 
		: producer_(std::move(producer))
	{
	}
	
	virtual spl::shared_ptr<draw_frame>							receive(int hints) override														{return producer_->receive(hints);}
	virtual spl::shared_ptr<draw_frame>							last_frame() const override		 												{return producer_->last_frame();}
	virtual std::wstring										print() const override															{return producer_->print();}
	virtual boost::property_tree::wptree 						info() const override															{return producer_->info();}
	virtual boost::unique_future<std::wstring>					call(const std::wstring& str) override											{return producer_->call(str);}
	virtual spl::shared_ptr<frame_producer>						get_following_producer() const override											{return producer_->get_following_producer();}
	virtual void												set_leading_producer(const spl::shared_ptr<frame_producer>& producer) override	{return producer_->set_leading_producer(producer);}
	virtual uint32_t											nb_frames() const override														{return producer_->nb_frames();}
};

class follow_producer_proxy : public producer_proxy_base
{	
public:
	follow_producer_proxy(spl::shared_ptr<frame_producer>&& producer) 
		: producer_proxy_base(std::move(producer))
	{
	}

	virtual spl::shared_ptr<draw_frame>	receive(int hints) override														
	{
		auto frame = producer_->receive(hints);
		if(frame == draw_frame::eof())
		{
			CASPAR_LOG(info) << producer_->print() << " End Of File.";
			auto following = producer_->get_following_producer();
			if(following != frame_producer::empty())
			{
				following->set_leading_producer(spl::make_shared_ptr(producer_));
				producer_ = std::move(following);
			}

			return receive(hints);
		}
		return frame;
	}

	virtual spl::shared_ptr<draw_frame> last_frame() const override 
	{
		return draw_frame::mute(producer_->last_frame());
	}
};

class destroy_producer_proxy : public producer_proxy_base
{	
public:
	destroy_producer_proxy(spl::shared_ptr<frame_producer>&& producer) 
		: producer_proxy_base(std::move(producer))
	{
	}

	~destroy_producer_proxy()
	{		
		static tbb::atomic<int> counter = tbb::atomic<int>();
		
		++counter;
		CASPAR_VERIFY(counter < 32);
		
		auto producer = new spl::shared_ptr<frame_producer>(std::move(producer_));
		async([=]
		{
			std::unique_ptr<spl::shared_ptr<frame_producer>> pointer_guard(producer);
			auto str = (*producer)->print();
			try
			{
				if(!producer->unique())
					CASPAR_LOG(trace) << str << L" Not destroyed on asynchronous destruction thread: " << producer->use_count();
				else
					CASPAR_LOG(trace) << str << L" Destroying on asynchronous destruction thread.";
			}
			catch(...){}

			pointer_guard.reset();

			--counter;
		}); 
	}
};

class print_producer_proxy : public producer_proxy_base
{	
public:
	print_producer_proxy(spl::shared_ptr<frame_producer>&& producer) 
		: producer_proxy_base(std::move(producer))
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
};

spl::shared_ptr<core::frame_producer> wrap_producer(spl::shared_ptr<core::frame_producer> producer)
{
	return spl::make_shared<follow_producer_proxy>(
			spl::make_shared<destroy_producer_proxy>(
			 spl::make_shared<print_producer_proxy>(
			  std::move(producer))));
}

spl::shared_ptr<core::frame_producer> do_create_producer(const spl::shared_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
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
		return producer;
		
	return producer;
}

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<frame_factory>& my_frame_factory, const std::vector<std::wstring>& params)
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
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax.") << arg_value_info(u8(str)));
	}

	return producer;
}


spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<frame_factory>& factory, const std::wstring& params)
{
	std::wstringstream iss(params);
	std::vector<std::wstring> tokens;
	typedef std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t> > iterator;
	std::copy(iterator(iss),  iterator(), std::back_inserter(tokens));
	return create_producer(factory, tokens);
}

}}