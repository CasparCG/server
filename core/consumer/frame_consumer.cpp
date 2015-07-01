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

#include "frame_consumer.h"

#include <common/except.h>
#include <common/future.h>

#include <core/video_format.h>
#include <core/frame/frame.h>

#include <boost/thread.hpp>

#include <future>
#include <vector>
#include <map>

namespace caspar { namespace core {

struct frame_consumer_registry::impl
{
	std::vector<consumer_factory_t> consumer_factories;
	std::map<std::wstring, preconfigured_consumer_factory_t> preconfigured_consumer_factories;
};

frame_consumer_registry::frame_consumer_registry()
	: impl_(new impl)
{
}

void frame_consumer_registry::register_consumer_factory(const consumer_factory_t& factory)
{
	impl_->consumer_factories.push_back(factory);
}

void frame_consumer_registry::register_preconfigured_consumer_factory(
		const std::wstring& element_name,
		const preconfigured_consumer_factory_t& factory)
{
	impl_->preconfigured_consumer_factories.insert(std::make_pair(element_name, factory));
}

class destroy_consumer_proxy : public frame_consumer
{	
	std::shared_ptr<frame_consumer> consumer_;
public:
	destroy_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer) 
		: consumer_(std::move(consumer))
	{
	}

	~destroy_consumer_proxy()
	{		
		static tbb::atomic<int> counter;
		static std::once_flag counter_init_once;
		std::call_once(counter_init_once, []{ counter = 0; });
			
		++counter;
		CASPAR_VERIFY(counter < 8);
		
		auto consumer = new std::shared_ptr<frame_consumer>(std::move(consumer_));
		boost::thread([=]
		{
			std::unique_ptr<std::shared_ptr<frame_consumer>> pointer_guard(consumer);

			auto str = (*consumer)->print();
			try
			{
				if(!consumer->unique())
					CASPAR_LOG(trace) << str << L" Not destroyed on asynchronous destruction thread: " << consumer->use_count();
				else
					CASPAR_LOG(trace) << str << L" Destroying on asynchronous destruction thread.";
			}
			catch(...){}

			pointer_guard.reset();

		}).detach(); 
	}
	
	std::future<bool> send(const_frame frame) override													{return consumer_->send(std::move(frame));}
	virtual void initialize(const video_format_desc& format_desc, int channel_index)	override	{return consumer_->initialize(format_desc, channel_index);}
	std::wstring print() const override																	{return consumer_->print();}	
	std::wstring name() const override																	{return consumer_->name();}
	boost::property_tree::wptree info() const override 													{return consumer_->info();}
	bool has_synchronization_clock() const override														{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override																	{return consumer_->buffer_depth();}
	int index() const override																			{return consumer_->index();}
	monitor::subject& monitor_output() override															{return consumer_->monitor_output();}										
};

class print_consumer_proxy : public frame_consumer
{	
	std::shared_ptr<frame_consumer> consumer_;
public:
	print_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer) 
		: consumer_(std::move(consumer))
	{
		CASPAR_LOG(info) << consumer_->print() << L" Initialized.";
	}

	~print_consumer_proxy()
	{		
		auto str = consumer_->print();
		CASPAR_LOG(trace) << str << L" Uninitializing.";
		consumer_.reset();
		CASPAR_LOG(info) << str << L" Uninitialized.";
	}
	
	std::future<bool> send(const_frame frame) override													{return consumer_->send(std::move(frame));}
	virtual void initialize(const video_format_desc& format_desc, int channel_index)	override	{return consumer_->initialize(format_desc, channel_index);}
	std::wstring print() const override																	{return consumer_->print();}
	std::wstring name() const override																	{return consumer_->name();}
	boost::property_tree::wptree info() const override 													{return consumer_->info();}
	bool has_synchronization_clock() const override														{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override																	{return consumer_->buffer_depth();}
	int index() const override																			{return consumer_->index();}
	monitor::subject& monitor_output() override															{return consumer_->monitor_output();}										
};

class recover_consumer_proxy : public frame_consumer
{	
	std::shared_ptr<frame_consumer> consumer_;
	int								channel_index_	= -1;
	video_format_desc				format_desc_;
public:
	recover_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer) 
		: consumer_(std::move(consumer))
	{
	}
	
	virtual std::future<bool> send(const_frame frame)					
	{
		try
		{
			return consumer_->send(frame);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			try
			{
				consumer_->initialize(format_desc_, channel_index_);
				return consumer_->send(frame);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(error) << print() << " Failed to recover consumer.";
				return make_ready_future(false);
			}
		}
	}

	virtual void initialize(const video_format_desc& format_desc, int channel_index)		
	{
		format_desc_	= format_desc;
		channel_index_	= channel_index;
		return consumer_->initialize(format_desc, channel_index);
	}

	std::wstring print() const override										{return consumer_->print();}
	std::wstring name() const override										{return consumer_->name();}
	boost::property_tree::wptree info() const override 						{return consumer_->info();}
	bool has_synchronization_clock() const override							{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override										{return consumer_->buffer_depth();}
	int index() const override												{return consumer_->index();}
	monitor::subject& monitor_output() override								{return consumer_->monitor_output();}										
};

// This class is used to guarantee that audio cadence is correct. This is important for NTSC audio.
class cadence_guard : public frame_consumer
{
	spl::shared_ptr<frame_consumer>		consumer_;
	std::vector<int>					audio_cadence_;
	video_format_desc					format_desc_;
	boost::circular_buffer<std::size_t>	sync_buffer_;
public:
	cadence_guard(const spl::shared_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}
	
	void initialize(const video_format_desc& format_desc, int channel_index) override
	{
		audio_cadence_	= format_desc.audio_cadence;
		sync_buffer_	= boost::circular_buffer<std::size_t>(format_desc.audio_cadence.size());
		format_desc_	= format_desc;
		consumer_->initialize(format_desc, channel_index);
	}

	std::future<bool> send(const_frame frame) override
	{		
		if(audio_cadence_.size() == 1)
			return consumer_->send(frame);

		std::future<bool> result = make_ready_future(true);
		
		if(boost::range::equal(sync_buffer_, audio_cadence_) && audio_cadence_.front() * format_desc_.audio_channels == static_cast<int>(frame.audio_data().size())) 
		{	
			// Audio sent so far is in sync, now we can send the next chunk.
			result = consumer_->send(frame);
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		}
		else
			CASPAR_LOG(trace) << print() << L" Syncing audio.";

		sync_buffer_.push_back(static_cast<int>(frame.audio_data().size() / format_desc_.audio_channels));
		
		return std::move(result);
	}
	
	std::wstring print() const override										{return consumer_->print();}
	std::wstring name() const override										{return consumer_->name();}
	boost::property_tree::wptree info() const override 						{return consumer_->info();}
	bool has_synchronization_clock() const override							{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override										{return consumer_->buffer_depth();}
	int index() const override												{return consumer_->index();}
	monitor::subject& monitor_output() override								{return consumer_->monitor_output();}										
};

spl::shared_ptr<core::frame_consumer> frame_consumer_registry::create_consumer(
		const std::vector<std::wstring>& params, interaction_sink* sink) const
{
	if(params.empty())
		CASPAR_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	auto consumer = frame_consumer::empty();
	auto& consumer_factories = impl_->consumer_factories;
	std::any_of(consumer_factories.begin(), consumer_factories.end(), [&](const consumer_factory_t& factory) -> bool
		{
			try
			{
				consumer = factory(params, sink);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return consumer != frame_consumer::empty();
		});

	if(consumer == frame_consumer::empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return spl::make_shared<destroy_consumer_proxy>(
			spl::make_shared<print_consumer_proxy>(
			 spl::make_shared<recover_consumer_proxy>(
			  spl::make_shared<cadence_guard>(
			   std::move(consumer)))));
}

spl::shared_ptr<frame_consumer> frame_consumer_registry::create_consumer(
		const std::wstring& element_name,
		const boost::property_tree::wptree& element,
		interaction_sink* sink) const
{
	auto& preconfigured_consumer_factories = impl_->preconfigured_consumer_factories;
	auto found = preconfigured_consumer_factories.find(element_name);

	if (found == preconfigured_consumer_factories.end())
		CASPAR_THROW_EXCEPTION(file_not_found()
			<< msg_info(L"No consumer factory registered for element name " + element_name));

	return spl::make_shared<destroy_consumer_proxy>(
			spl::make_shared<print_consumer_proxy>(
					spl::make_shared<recover_consumer_proxy>(
							spl::make_shared<cadence_guard>(
									found->second(element, sink)))));
}

const spl::shared_ptr<frame_consumer>& frame_consumer::empty()
{
	class empty_frame_consumer : public frame_consumer
	{
	public:
		std::future<bool> send(const_frame) override { return make_ready_future(false); }
		void initialize(const video_format_desc&, int) override{}
		std::wstring print() const override {return L"empty";}
		std::wstring name() const override {return L"empty";}
		bool has_synchronization_clock() const override {return false;}
		int buffer_depth() const override {return 0;};
		virtual int index() const{return -1;}
		monitor::subject& monitor_output() override {static monitor::subject monitor_subject(""); return monitor_subject;}										
		boost::property_tree::wptree info() const override
		{
			boost::property_tree::wptree info;
			info.add(L"type", L"empty");
			return info;
		}
	};
	static spl::shared_ptr<frame_consumer> consumer = spl::make_shared<empty_frame_consumer>();
	return consumer;
}

}}
