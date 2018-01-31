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
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/video_format.h>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <common/except.h>
#include <common/memory.h>
#include <common/semaphore.h>
#include <common/future.h>

#include <tbb/concurrent_queue.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include <queue>

namespace caspar { namespace reroute {

class channel_consumer : public core::frame_consumer
{
	core::monitor::subject								monitor_subject_;
	tbb::concurrent_bounded_queue<core::const_frame>	frame_buffer_;
	core::video_format_desc								format_desc_;
	int													channel_index_;
	int													consumer_index_;
	std::atomic<bool>									is_running_;
	std::atomic<int64_t>								current_age_;
	semaphore											frames_available_ { 0 };
	int													frames_delay_;

public:
	channel_consumer(int frames_delay)
		: consumer_index_(next_consumer_index())
		, frames_delay_(frames_delay)
	{
		is_running_ = true;
		current_age_ = 0;
		frame_buffer_.set_capacity(3 + frames_delay);
	}

	static int next_consumer_index()
	{
		static std::atomic<int> consumer_index_counter;
		static std::once_flag consumer_index_counter_initialized;

		std::call_once(consumer_index_counter_initialized, [&]()
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

		if (pushed)
			frames_available_.release();

		return make_ready_future(is_running_.load());
	}

	void initialize(const core::video_format_desc& format_desc,	int channel_index) override
	{
		format_desc_    = format_desc;
		channel_index_  = channel_index;
	}

	std::wstring name() const override
	{
		return L"channel-consumer";
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

	void block_until_first_frame_available()
	{
		if (!frames_available_.try_acquire(1 + frames_delay_, std::chrono::seconds(2)))
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

core::video_format_desc get_progressive_format(core::video_format_desc format_desc)
{
	if (format_desc.field_count == 1)
		return format_desc;

	format_desc.framerate		*= 2;
	format_desc.fps				*= 2.0;
	format_desc.audio_cadence	 = core::find_audio_cadence(format_desc.framerate);
	format_desc.time_scale		*= 2;
	format_desc.field_count		 = 1;

	return format_desc;
}

class channel_producer : public core::frame_producer_base
{
	core::monitor::subject						monitor_subject_;

	const spl::shared_ptr<core::frame_factory>	frame_factory_;
	const core::video_format_desc				format_desc_;
	const spl::shared_ptr<channel_consumer>		consumer_;

	std::queue<core::draw_frame>				frame_buffer_;

public:
	explicit channel_producer(
			const core::frame_producer_dependencies& dependecies,
			const spl::shared_ptr<core::video_channel>& channel,
			int frames_delay,
			bool no_auto_deinterlace)
		: frame_factory_(dependecies.frame_factory)
		, format_desc_(dependecies.format_desc)
		, consumer_(spl::make_shared<channel_consumer>(frames_delay))
	{
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
        return core::draw_frame(consumer_->receive());
	}

	std::wstring name() const override
	{
		return L"channel-producer";
	}

	std::wstring print() const override
	{
		return L"channel-producer[]";
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

	boost::rational<int> current_framerate() const
	{
        return format_desc_.framerate;
	}
};

spl::shared_ptr<core::frame_producer> create_channel_producer(
		const core::frame_producer_dependencies& dependencies,
		const spl::shared_ptr<core::video_channel>& channel,
		int frames_delay,
		bool no_auto_deinterlace)
{
	auto producer = spl::make_shared<channel_producer>(dependencies, channel, frames_delay, no_auto_deinterlace);
    return core::create_destroy_proxy(producer);
}

}}
