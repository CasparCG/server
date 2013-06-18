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

#include "../../stdafx.h"

#include "layer_producer.h"

#include "../../consumer/write_frame_consumer.h"
#include "../../consumer/output.h"
#include "../../video_channel.h"

#include "../stage.h"
#include "../frame/basic_frame.h"
#include "../frame/frame_factory.h"
#include "../../mixer/write_frame.h"
#include "../../mixer/read_frame.h"

#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/concurrency/future_util.h>

#include <boost/format.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace core {

class layer_consumer : public write_frame_consumer
{	
	tbb::concurrent_bounded_queue<safe_ptr<basic_frame>>	frame_buffer_;

public:
	layer_consumer() 
	{
		frame_buffer_.set_capacity(2);
	}

	~layer_consumer()
	{
	}

	// write_frame_consumer

	virtual void send(const safe_ptr<basic_frame>& src_frame) override
	{
		frame_buffer_.try_push(src_frame);
	}

	virtual std::wstring print() const override
	{
		return L"[layer_consumer]";
	}

	safe_ptr<basic_frame> receive()
	{
		safe_ptr<basic_frame> frame;
		if (!frame_buffer_.try_pop(frame))
		{
			return basic_frame::late();
		}
		return frame;
	}
};

class layer_producer : public frame_producer
{
	monitor::subject					monitor_subject_;

	const safe_ptr<frame_factory>			frame_factory_;
	int										layer_;
	const std::shared_ptr<layer_consumer>	consumer_;

	safe_ptr<basic_frame>					last_frame_;
	uint64_t								frame_number_;

	const safe_ptr<stage>                   stage_;

public:
	explicit layer_producer(const safe_ptr<frame_factory>& frame_factory, const safe_ptr<stage>& stage, int layer) 
		: frame_factory_(frame_factory)
		, layer_(layer)
		, stage_(stage)
		, consumer_(new layer_consumer())
		, last_frame_(basic_frame::empty())
		, frame_number_(0)
	{
		stage_->add_layer_consumer(this, layer_, consumer_);
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~layer_producer()
	{
		stage_->remove_layer_consumer(this, layer_);
		CASPAR_LOG(info) << print() << L" Uninitialized";
	}

	// frame_producer
			
	virtual safe_ptr<basic_frame> receive(int) override
	{
		auto consumer_frame = consumer_->receive();
		if (consumer_frame == basic_frame::late())
			return last_frame_;

		frame_number_++;
		return last_frame_ = consumer_frame;
	}	

	virtual safe_ptr<basic_frame> last_frame() const override
	{
		return last_frame_; 
	}	

	virtual std::wstring print() const override
	{
		return L"layer-producer[" + boost::lexical_cast<std::wstring>(layer_) + L"]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"layer-producer");
		return info;
	}

	monitor::source& monitor_output() 
	{
		return monitor_subject_;
	}

};

safe_ptr<frame_producer> create_layer_producer(const safe_ptr<core::frame_factory>& frame_factory, const safe_ptr<stage>& stage, int layer)
{
	return create_producer_print_proxy(
		make_safe<layer_producer>(frame_factory, stage, layer)
	);
}

}}