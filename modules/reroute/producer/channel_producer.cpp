/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../stdafx.h"

#include "channel_producer.h"

#include <core/monitor/monitor.h>
#include <core/consumer/frame_consumer.h>
#include <core/consumer/output.h>
#include <core/producer/frame_producer.h>
#include <core/video_channel.h>

#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>
#include <core/frame/audio_channel_layout.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/video_format.h>

#include <boost/thread/once.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <common/except.h>
#include <common/memory.h>
#include <common/future.h>
#include <common/memcpy.h>

#include <tbb/concurrent_queue.h>

#include <asmlib.h>

#include <queue>

namespace caspar { namespace reroute {

class channel_consumer : public core::frame_consumer
{	
	core::monitor::subject								monitor_subject_;
	tbb::concurrent_bounded_queue<core::const_frame>	frame_buffer_;
	core::video_format_desc								format_desc_;
	core::audio_channel_layout							channel_layout_			= core::audio_channel_layout::invalid();
	int													channel_index_;
	int													consumer_index_;
	tbb::atomic<bool>									is_running_;
	tbb::atomic<int64_t>								current_age_;
	std::promise<void>									first_frame_promise_;
	std::future<void>									first_frame_available_;
	bool												first_frame_reported_;

public:
	channel_consumer()
		: consumer_index_(next_consumer_index())
		, first_frame_available_(first_frame_promise_.get_future())
		, first_frame_reported_(false)
	{
		is_running_ = true;
		current_age_ = 0;
		frame_buffer_.set_capacity(3);
	}

	static int next_consumer_index()
	{
		static tbb::atomic<int> consumer_index_counter;
		static boost::once_flag consumer_index_counter_initialized;

		boost::call_once(consumer_index_counter_initialized, [&]()
		{
			consumer_index_counter = 0;
		});

		return ++consumer_index_counter;
	}

	~channel_consumer()
	{
	}

	// frame_consumer

	std::future<bool> send(core::const_frame frame) override
	{
		bool pushed = frame_buffer_.try_push(frame);

		if (pushed && !first_frame_reported_)
		{
			first_frame_promise_.set_value();
			first_frame_reported_ = true;
		}

		return make_ready_future(is_running_.load());
	}

	void initialize(
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& channel_layout,
			int channel_index) override
	{
		format_desc_    = format_desc;
		channel_layout_ = channel_layout;
		channel_index_  = channel_index;
	}

	std::wstring name() const override
	{
		return L"channel-consumer";
	}

	int64_t presentation_frame_age_millis() const override
	{
		return current_age_;
	}

	std::wstring print() const override
	{
		return L"[channel-consumer|" + boost::lexical_cast<std::wstring>(channel_index_) + L"]";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"channel-consumer");
		info.add(L"channel-index", channel_index_);
		return info;
	}
	
	bool has_synchronization_clock() const override
	{
		return false;
	}

	int buffer_depth() const override
	{
		return -1;
	}

	int index() const override
	{
		return 78500 + consumer_index_;
	}

	core::monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}

	// channel_consumer
	
	const core::video_format_desc& get_video_format_desc()
	{
		return format_desc_;
	}

	const core::audio_channel_layout& get_audio_channel_layout()
	{
		return channel_layout_;
	}

	void block_until_first_frame_available()
	{
		if (first_frame_available_.wait_for(std::chrono::seconds(2)) == std::future_status::timeout)
			CASPAR_LOG(warning)
					<< print() << L" Timed out while waiting for first frame";
	}

	core::const_frame receive()
	{
		core::const_frame frame = core::const_frame::empty();

		if (!is_running_)
			return frame;
		
		if (frame_buffer_.try_pop(frame))
			current_age_ = frame.get_age_millis();

		return frame;
	}

	void stop()
	{
		is_running_ = false;
	}
};
	
class channel_producer : public core::frame_producer_base
{
	core::monitor::subject						monitor_subject_;

	const spl::shared_ptr<core::frame_factory>	frame_factory_;
	const core::video_format_desc				output_format_desc_;
	const spl::shared_ptr<channel_consumer>		consumer_;
	core::constraints							pixel_constraints_;

	std::queue<core::draw_frame>				frame_buffer_;
	uint64_t									frame_number_;

public:
	explicit channel_producer(const core::frame_producer_dependencies& dependecies, const spl::shared_ptr<core::video_channel>& channel) 
		: frame_factory_(dependecies.frame_factory)
		, output_format_desc_(dependecies.format_desc)
		, frame_number_(0)
	{
		pixel_constraints_.width.set(output_format_desc_.width);
		pixel_constraints_.height.set(output_format_desc_.height);
		channel->output().add(consumer_);
		consumer_->block_until_first_frame_available();
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~channel_producer()
	{
		consumer_->stop();
		CASPAR_LOG(info) << print() << L" Uninitialized";
	}

	// frame_producer
			
	core::draw_frame receive_impl() override
	{
		auto format_desc = consumer_->get_video_format_desc();

		if(frame_buffer_.size() > 0)
		{
			auto frame = frame_buffer_.front();
			frame_buffer_.pop();
			return frame;
		}
		
		auto read_frame = consumer_->receive();
		if(read_frame == core::const_frame::empty() || read_frame.image_data().empty())
			return core::draw_frame::late();

		frame_number_++;
		
		bool double_speed	= std::abs(output_format_desc_.fps / 2.0 - format_desc.fps) < 0.01;
		bool half_speed		= std::abs(format_desc.fps / 2.0 - output_format_desc_.fps) < 0.01;

		if(half_speed && frame_number_ % 2 == 0) // Skip frame
			return receive_impl();

		core::pixel_format_desc desc;
		desc.format = core::pixel_format::bgra;
		desc.planes.push_back(core::pixel_format_desc::plane(format_desc.width, format_desc.height, 4));
		auto frame = frame_factory_->create_frame(this, desc, consumer_->get_audio_channel_layout());

		bool copy_audio = !double_speed && !half_speed;

		if (copy_audio)
		{
			frame.audio_data().reserve(read_frame.audio_data().size());
			boost::copy(read_frame.audio_data(), std::back_inserter(frame.audio_data()));
		}

		fast_memcpy(frame.image_data().begin(), read_frame.image_data().begin(), read_frame.image_data().size());

		frame_buffer_.push(core::draw_frame(std::move(frame)));
		
		if(double_speed)
			frame_buffer_.push(frame_buffer_.back());

		return receive_impl();
	}	

	std::wstring name() const override
	{
		return L"channel-producer";
	}

	std::wstring print() const override
	{
		return L"channel-producer[]";
	}

	core::constraints& pixel_constraints() override
	{
		return pixel_constraints_;
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"channel-producer");
		return info;
	}

	core::monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}
};

spl::shared_ptr<core::frame_producer> create_channel_producer(
		const core::frame_producer_dependencies& dependencies,
		const spl::shared_ptr<core::video_channel>& channel)
{
	return spl::make_shared<channel_producer>(dependencies, channel);
}

}}