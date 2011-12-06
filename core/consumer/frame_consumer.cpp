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

#include <common/env.h>
#include <common/memory/safe_ptr.h>
#include <common/exception/exceptions.h>
#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include <boost/circular_buffer.hpp>

namespace caspar { namespace core {
		
std::vector<const consumer_factory_t> g_factories;

void register_consumer_factory(const consumer_factory_t& factory)
{
	g_factories.push_back(factory);
}

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::string>& params)
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

	return consumer;
}

// This class is used to guarantee that audio cadence is correct. This is important for NTSC audio.
class cadence_guard : public frame_consumer
{
	safe_ptr<frame_consumer>		consumer_;
	std::vector<size_t>				audio_cadence_;
	boost::circular_buffer<size_t>	sync_buffer_;
	bool							synced_;
public:
	cadence_guard(const safe_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}
	
	virtual void initialize(const video_format_desc& format_desc, int channel_index) override
	{
		audio_cadence_	= format_desc.audio_cadence;
		sync_buffer_	= boost::circular_buffer<size_t>(format_desc.audio_cadence.size());
		consumer_->initialize(format_desc, channel_index);
	}

	virtual bool send(const safe_ptr<read_frame>& frame) override
	{		
		sync_buffer_.push_back(static_cast<size_t>(frame->audio_data().size()));
		
		if(!boost::range::equal(sync_buffer_, audio_cadence_))
		{
			synced_ = false;
			CASPAR_LOG(trace) << "[cadence_guard] Audio cadence unsynced. Skipping frame.";
			return true;
		}
		else if(!synced_)
		{
			synced_ = true;
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
			return true;
		}

		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);

		return consumer_->send(frame);
	}

	virtual std::string print() const override
	{
		return consumer_->print();
	}

	virtual boost::property_tree::ptree info() const override
	{
		return consumer_->info();
	}

	virtual bool has_synchronization_clock() const override
	{
		return consumer_->has_synchronization_clock();
	}

	virtual size_t buffer_depth() const override
	{
		return consumer_->buffer_depth();
	}

	virtual int index() const override
	{
		return consumer_->index();
	}
};

safe_ptr<frame_consumer> create_consumer_cadence_guard(const safe_ptr<frame_consumer>& consumer)
{
	return make_safe<cadence_guard>(std::move(consumer));
}

const safe_ptr<frame_consumer>& frame_consumer::empty()
{
	struct empty_frame_consumer : public frame_consumer
	{
		virtual bool send(const safe_ptr<read_frame>&) override {return false;}
		virtual void initialize(const video_format_desc&, int) override{}
		virtual std::string print() const override {return "empty";}
		virtual bool has_synchronization_clock() const override {return false;}
		virtual size_t buffer_depth() const override {return 0;};
		virtual int index() const{return -1;}
		virtual boost::property_tree::ptree info() const override
		{
			boost::property_tree::ptree info;
			info.add("type", "empty-consumer");
			return info;
		}
	};
	static safe_ptr<frame_consumer> consumer = make_safe<empty_frame_consumer>();
	return consumer;
}

}}