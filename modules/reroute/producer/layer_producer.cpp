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
#include <core/producer/framerate/framerate_producer.h>

#include <common/except.h>
#include <common/semaphore.h>

#include <boost/format.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace reroute {

class layer_consumer : public core::write_frame_consumer
{
	tbb::concurrent_bounded_queue<core::draw_frame>	frame_buffer_;
	semaphore										frames_available_ { 0 };
	int												frames_delay_;

public:
	layer_consumer(int frames_delay)
		: frames_delay_(frames_delay)
	{
		frame_buffer_.set_capacity(2 + frames_delay);
	}

	~layer_consumer()
	{
	}

	// write_frame_consumer

	void send(const core::draw_frame& src_frame) override
	{
		bool pushed = frame_buffer_.try_push(src_frame);

		if (pushed)
			frames_available_.release();
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
		if (!frames_available_.try_acquire(1 + frames_delay_, boost::chrono::seconds(2)))
			CASPAR_LOG(warning)
					<< print() << L" Timed out while waiting for first frame";
	}
};

std::vector<core::draw_frame> extract_actual_frames(core::draw_frame original, core::field_mode field_order)
{
	if (field_order == core::field_mode::progressive)
		return { std::move(original) };

	struct field_extractor : public core::frame_visitor
	{
		std::vector<core::draw_frame>	fields;
		core::field_mode				field_order_first;
		core::field_mode				visiting_mode = core::field_mode::progressive;

		field_extractor(core::field_mode field_order_)
			: field_order_first(field_order_)
		{
		}

		void push(const core::frame_transform& transform) override
		{
			if (visiting_mode == core::field_mode::progressive)
				visiting_mode = transform.image_transform.field_mode;
		}

		void visit(const core::const_frame& frame) override
		{
			if (visiting_mode == field_order_first && fields.size() == 0)
				fields.push_back(core::draw_frame(core::const_frame(frame)));
			else if (visiting_mode != core::field_mode::progressive && visiting_mode != field_order_first && fields.size() == 1)
				fields.push_back(core::draw_frame(core::const_frame(frame)));
		}

		void pop() override
		{
			visiting_mode = core::field_mode::progressive;
		};
	};

	field_extractor extractor(field_order);
	original.accept(extractor);

	if (extractor.fields.size() == 2)
		return std::move(extractor.fields);
	else
		return { std::move(original) };
}

class layer_producer : public core::frame_producer_base
{
	core::monitor::subject						monitor_subject_;

	const int									layer_;
	const spl::shared_ptr<layer_consumer>		consumer_;

	core::draw_frame							last_frame_;

	const std::weak_ptr<core::video_channel>	channel_;
	core::constraints							pixel_constraints_;

	tbb::atomic<bool>							double_framerate_;
	std::queue<core::draw_frame>				frame_buffer_;

public:
	explicit layer_producer(const spl::shared_ptr<core::video_channel>& channel, int layer, int frames_delay)
		: layer_(layer)
		, consumer_(spl::make_shared<layer_consumer>(frames_delay))
		, channel_(channel)
		, last_frame_(core::draw_frame::late())
	{
		pixel_constraints_.width.set(channel->video_format_desc().width);
		pixel_constraints_.height.set(channel->video_format_desc().height);
		channel->stage().add_layer_consumer(this, layer_, consumer_);
		consumer_->block_until_first_frame_available();
		double_framerate_ = false;
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~layer_producer()
	{
		auto channel = channel_.lock();

		if (channel)
			channel->stage().remove_layer_consumer(this, layer_);

		CASPAR_LOG(info) << print() << L" Uninitialized";
	}

	// frame_producer

	core::draw_frame receive_impl() override
	{
		if (!frame_buffer_.empty())
		{
			last_frame_ = frame_buffer_.front();
			frame_buffer_.pop();
			return last_frame_;
		}

		auto channel = channel_.lock();

		if (!channel)
			return last_frame_;

		auto consumer_frame = consumer_->receive();
		if (consumer_frame == core::draw_frame::late())
			return last_frame_;

		auto actual_frames = extract_actual_frames(std::move(consumer_frame), channel->video_format_desc().field_mode);
		double_framerate_ = actual_frames.size() == 2;

		for (auto& frame : actual_frames)
			frame_buffer_.push(std::move(frame));

		return receive_impl();
	}

	core::draw_frame last_frame() override
	{
		return last_frame_;
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

	boost::rational<int> current_framerate() const
	{
		auto channel = channel_.lock();

		if (!channel)
			return 1;

		return double_framerate_
				? channel->video_format_desc().framerate * 2
				: channel->video_format_desc().framerate;
	}
};

spl::shared_ptr<core::frame_producer> create_layer_producer(
		const spl::shared_ptr<core::video_channel>& channel,
		int layer,
		int frames_delay,
		const core::video_format_desc& destination_mode)
{
	auto producer = spl::make_shared<layer_producer>(channel, layer, frames_delay);

	return core::create_framerate_producer(
			producer,
			std::bind(&layer_producer::current_framerate, producer),
			destination_mode.framerate,
			destination_mode.field_mode,
			destination_mode.audio_cadence);
}

}}
