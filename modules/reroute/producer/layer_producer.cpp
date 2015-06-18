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
* Author: Cambell Prince, cambell.prince@gmail.com
*/

#include "../stdafx.h"

#include "layer_producer.h"

#include <core/consumer/write_frame_consumer.h>
#include <core/consumer/output.h>
#include <core/video_channel.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>

#include <common/except.h>
#include <common/future.h>

#include <boost/format.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace reroute {

class layer_consumer : public core::write_frame_consumer
{	
	tbb::concurrent_bounded_queue<core::draw_frame>	frame_buffer_;
	std::promise<void>								first_frame_promise_;
	std::future<void>								first_frame_available_;
	bool											first_frame_reported_;

public:
	layer_consumer()
		: first_frame_available_(first_frame_promise_.get_future())
		, first_frame_reported_(false)
	{
		frame_buffer_.set_capacity(2);
	}

	~layer_consumer()
	{
	}

	// write_frame_consumer

	void send(const core::draw_frame& src_frame) override
	{
		bool pushed = frame_buffer_.try_push(src_frame);

		if (pushed && !first_frame_reported_)
		{
			first_frame_promise_.set_value();
			first_frame_reported_ = true;
		}
	}

	std::wstring print() const override
	{
		return L"[layer_consumer]";
	}

	core::draw_frame receive()
	{
		core::draw_frame frame;
		if (!frame_buffer_.try_pop(frame))
		{
			return core::draw_frame::late();
		}
		return frame;
	}

	void block_until_first_frame_available()
	{
		if (first_frame_available_.wait_for(std::chrono::seconds(2)) == std::future_status::timeout)
			CASPAR_LOG(warning)
					<< print() << L" Timed out while waiting for first frame";
	}
};

class layer_producer : public core::frame_producer_base
{
	core::monitor::subject						monitor_subject_;

	const int									layer_;
	const spl::shared_ptr<layer_consumer>		consumer_;

	core::draw_frame							last_frame_;
	uint64_t									frame_number_;

	const spl::shared_ptr<core::video_channel>	channel_;
	core::constraints							pixel_constraints_;

public:
	explicit layer_producer(const spl::shared_ptr<core::video_channel>& channel, int layer) 
		: layer_(layer)
		, channel_(channel)
		, last_frame_(core::draw_frame::empty())
		, frame_number_(0)
	{
		pixel_constraints_.width.set(channel->video_format_desc().width);
		pixel_constraints_.height.set(channel->video_format_desc().height);
		channel_->stage().add_layer_consumer(this, layer_, consumer_);
		consumer_->block_until_first_frame_available();
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~layer_producer()
	{
		channel_->stage().remove_layer_consumer(this, layer_);
		CASPAR_LOG(info) << print() << L" Uninitialized";
	}

	// frame_producer
			
	core::draw_frame receive_impl() override
	{
		auto consumer_frame = consumer_->receive();
		if (consumer_frame == core::draw_frame::late())
			return last_frame_;

		frame_number_++;
		return last_frame_ = consumer_frame;
	}	

	std::wstring print() const override
	{
		return L"layer-producer[" + boost::lexical_cast<std::wstring>(layer_) + L"]";
	}

	std::wstring name() const override
	{
		return L"layer-producer";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"layer-producer");
		return info;
	}

	core::monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}

	core::constraints& pixel_constraints() override
	{
		return pixel_constraints_;
	}
};

spl::shared_ptr<core::frame_producer> create_layer_producer(const spl::shared_ptr<core::video_channel>& channel, int layer)
{
	return spl::make_shared<layer_producer>(channel, layer);
}

}}