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

#include "../../stdafx.h"

#include "channel_producer.h"

#include "../../consumer/frame_consumer.h"
#include "../../consumer/output.h"
#include "../../video_channel.h"

#include "../frame/basic_frame.h"
#include "../frame/frame_factory.h"
#include "../../mixer/write_frame.h"
#include "../../mixer/read_frame.h"

#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/concurrency/future_util.h>
#include <tbb/concurrent_queue.h>

namespace caspar { namespace core {

class channel_consumer : public frame_consumer
{	
	tbb::concurrent_bounded_queue<std::shared_ptr<read_frame>>	frame_buffer_;
	core::video_format_desc										format_desc_;
	int															channel_index_;
	tbb::atomic<bool>											is_running_;

public:
	channel_consumer() 
	{
		is_running_ = true;
		frame_buffer_.set_capacity(3);
	}

	~channel_consumer()
	{
		stop();
	}

	// frame_consumer

	virtual boost::unique_future<bool> send(const safe_ptr<read_frame>& frame) override
	{
		frame_buffer_.try_push(frame);
		return caspar::wrap_as_future(is_running_.load());
	}

	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		format_desc_    = format_desc;
		channel_index_  = channel_index;
	}

	virtual std::wstring print() const override
	{
		return L"[channel-consumer|" + boost::lexical_cast<std::wstring>(channel_index_) + L"]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"channel-consumer");
		info.add(L"channel-index", channel_index_);
		return info;
	}
	
	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual size_t buffer_depth() const override
	{
		return 1;
	}

	virtual int index() const override
	{
		return 78500 + channel_index_;
	}

	// channel_consumer

	void stop()
	{
		is_running_ = false;
		frame_buffer_.try_push(make_safe<read_frame>());
	}
	
	const core::video_format_desc& get_video_format_desc()
	{
		return format_desc_;
	}

	std::shared_ptr<read_frame> receive()
	{
		if(!is_running_)
			return make_safe<read_frame>();
		std::shared_ptr<read_frame> frame;
		frame_buffer_.try_pop(frame);
		return frame;
	}
};
	
class channel_producer : public frame_producer
{
	const safe_ptr<frame_factory>		frame_factory_;
	const safe_ptr<channel_consumer>	consumer_;

	std::queue<safe_ptr<basic_frame>>	frame_buffer_;
	safe_ptr<basic_frame>				last_frame_;
	uint64_t							frame_number_;

public:
	explicit channel_producer(const safe_ptr<frame_factory>& frame_factory, const safe_ptr<video_channel>& channel) 
		: frame_factory_(frame_factory)
		, consumer_(make_safe<channel_consumer>())
		, last_frame_(basic_frame::empty())
		, frame_number_(0)
	{
		channel->output()->add(consumer_);
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~channel_producer()
	{
		consumer_->stop();
		CASPAR_LOG(info) << print() << L" Uninitialized";
	}

	// frame_producer
			
	virtual safe_ptr<basic_frame> receive(int) override
	{
		auto format_desc = consumer_->get_video_format_desc();

		if(frame_buffer_.size() > 1)
		{
			auto frame = frame_buffer_.front();
			frame_buffer_.pop();
			return last_frame_ = frame;
		}
		
		auto read_frame = consumer_->receive();
		if(!read_frame || read_frame->image_data().empty())
			return basic_frame::late();		

		frame_number_++;
		
		core::pixel_format_desc desc;
		bool double_speed	= std::abs(frame_factory_->get_video_format_desc().fps / 2.0 - format_desc.fps) < 0.01;		
		bool half_speed		= std::abs(format_desc.fps / 2.0 - frame_factory_->get_video_format_desc().fps) < 0.01;

		if(half_speed && frame_number_ % 2 == 0) // Skip frame
			return receive(0);

		desc.pix_fmt = core::pixel_format::bgra;
		desc.planes.push_back(core::pixel_format_desc::plane(format_desc.width, format_desc.height, 4));
		auto frame = frame_factory_->create_frame(this, desc);

		fast_memcpy(frame->image_data().begin(), read_frame->image_data().begin(), read_frame->image_data().size());
		frame->commit();

		frame_buffer_.push(frame);	
		
		if(double_speed)	
			frame_buffer_.push(frame);

		return receive(0);
	}	

	virtual safe_ptr<basic_frame> last_frame() const override
	{
		return last_frame_; 
	}	

	virtual std::wstring print() const override
	{
		return L"channel[]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"channel-producer");
		return info;
	}
};

safe_ptr<frame_producer> create_channel_producer(const safe_ptr<core::frame_factory>& frame_factory, const safe_ptr<video_channel>& channel)
{
	return create_producer_print_proxy(
			make_safe<channel_producer>(frame_factory, channel));
}

}}