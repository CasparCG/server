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

#include <core/video_format.h>
#include <core/frame/data_frame.h>

#include <common/concurrency/async.h>

namespace caspar { namespace core {
		
std::vector<const consumer_factory_t> g_factories;

void register_consumer_factory(const consumer_factory_t& factory)
{
	g_factories.push_back(factory);
}

class destroy_consumer_proxy : public frame_consumer
{	
	std::unique_ptr<std::shared_ptr<frame_consumer>> consumer_;
public:
	destroy_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer) 
		: consumer_(new std::shared_ptr<frame_consumer>(std::move(consumer)))
	{
	}

	~destroy_consumer_proxy()
	{		
		static tbb::atomic<int> counter = tbb::atomic<int>();
			
		++counter;
		CASPAR_VERIFY(counter < 32);
		
		auto consumer = consumer_.release();
		async([=]
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

			--counter;
		}); 
	}
	
	virtual bool send(const spl::shared_ptr<const struct data_frame>& frame) override					{return (*consumer_)->send(frame);}
	virtual void initialize(const struct video_format_desc& format_desc, int channel_index)	override	{return (*consumer_)->initialize(format_desc, channel_index);}
	virtual std::wstring print() const override															{return (*consumer_)->print();}
	virtual boost::property_tree::wptree info() const override 											{return (*consumer_)->info();}
	virtual bool has_synchronization_clock() const override												{return (*consumer_)->has_synchronization_clock();}
	virtual int buffer_depth() const override															{return (*consumer_)->buffer_depth();}
	virtual int index() const override																	{return (*consumer_)->index();}
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
	
	virtual bool send(const spl::shared_ptr<const struct data_frame>& frame) override					{return consumer_->send(frame);}
	virtual void initialize(const struct video_format_desc& format_desc, int channel_index)	override	{return consumer_->initialize(format_desc, channel_index);}
	virtual std::wstring print() const override															{return consumer_->print();}
	virtual boost::property_tree::wptree info() const override 											{return consumer_->info();}
	virtual bool has_synchronization_clock() const override												{return consumer_->has_synchronization_clock();}
	virtual int buffer_depth() const override															{return consumer_->buffer_depth();}
	virtual int index() const override																	{return consumer_->index();}
};

// This class is used to guarantee that audio cadence is correct. This is important for NTSC audio.
class cadence_guard : public frame_consumer
{
	spl::shared_ptr<frame_consumer>		consumer_;
	std::vector<int>				audio_cadence_;
	boost::circular_buffer<int>	sync_buffer_;
public:
	cadence_guard(const spl::shared_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}
	
	virtual void initialize(const video_format_desc& format_desc, int channel_index) override
	{
		audio_cadence_	= format_desc.audio_cadence;
		sync_buffer_	= boost::circular_buffer<int>(format_desc.audio_cadence.size());
		consumer_->initialize(format_desc, channel_index);
	}

	virtual bool send(const spl::shared_ptr<const data_frame>& frame) override
	{		
		if(audio_cadence_.size() == 1)
			return consumer_->send(frame);

		bool result = true;
		
		if(boost::range::equal(sync_buffer_, audio_cadence_) && audio_cadence_.front() == static_cast<int>(frame->audio_data().size())) 
		{	
			// Audio sent so far is in sync, now we can send the next chunk.
			result = consumer_->send(frame);
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		}
		else
			CASPAR_LOG(trace) << print() << L" Syncing audio.";

		sync_buffer_.push_back(static_cast<int>(frame->audio_data().size()));
		
		return result;
	}
	
	virtual std::wstring print() const override					{return consumer_->print();}
	virtual boost::property_tree::wptree info() const override 	{return consumer_->info();}
	virtual bool has_synchronization_clock() const override		{return consumer_->has_synchronization_clock();}
	virtual int buffer_depth() const override					{return consumer_->buffer_depth();}
	virtual int index() const override							{return consumer_->index();}
};

class recover_consumer_proxy : public frame_consumer
{	
	std::shared_ptr<frame_consumer> consumer_;
	int								channel_index_;
	video_format_desc				format_desc_;
public:
	recover_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer) 
		: consumer_(std::move(consumer))
	{
	}
	
	virtual bool send(const spl::shared_ptr<const struct data_frame>& frame)					
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
				CASPAR_LOG(error) << print() << "Failed to recover consumer.";
				return false;
			}
		}
	}

	virtual void initialize(const struct video_format_desc& format_desc, int channel_index)		
	{
		format_desc_	= format_desc;
		channel_index_	= channel_index;
		return consumer_->initialize(format_desc, channel_index);
	}

	virtual std::wstring print() const override					{return consumer_->print();}
	virtual boost::property_tree::wptree info() const override 	{return consumer_->info();}
	virtual bool has_synchronization_clock() const override		{return consumer_->has_synchronization_clock();}
	virtual int buffer_depth() const override					{return consumer_->buffer_depth();}
	virtual int index() const override							{return consumer_->index();}
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	auto consumer = frame_consumer::empty();
	std::any_of(g_factories.begin(), g_factories.end(), [&](const consumer_factory_t& factory) -> bool
		{
			try
			{
				consumer = factory(params);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return consumer != frame_consumer::empty();
		});

	if(consumer == frame_consumer::empty())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return spl::make_shared<destroy_consumer_proxy>(
			spl::make_shared<print_consumer_proxy>(
			 spl::make_shared<recover_consumer_proxy>(
			  spl::make_shared<cadence_guard>(
			   std::move(consumer)))));
}

const spl::shared_ptr<frame_consumer>& frame_consumer::empty()
{
	struct empty_frame_consumer : public frame_consumer
	{
		virtual bool send(const spl::shared_ptr<const data_frame>&) override {return false;}
		virtual void initialize(const video_format_desc&, int) override{}
		virtual std::wstring print() const override {return L"empty";}
		virtual bool has_synchronization_clock() const override {return false;}
		virtual int buffer_depth() const override {return 0;};
		virtual int index() const{return -1;}
		virtual boost::property_tree::wptree info() const override
		{
			boost::property_tree::wptree info;
			info.add(L"type", L"empty-consumer");
			return info;
		}
	};
	static spl::shared_ptr<frame_consumer> consumer = spl::make_shared<empty_frame_consumer>();
	return consumer;
}

}}