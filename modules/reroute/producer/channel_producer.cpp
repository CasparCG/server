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
#include <core/producer/framerate/framerate_producer.h>
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
#include <common/memcpy.h>
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

#include <modules/ffmpeg/producer/muxer/frame_muxer.h>
#include <modules/ffmpeg/producer/util/util.h>

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

		if (pushed)
			frames_available_.release();

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
		if (!frames_available_.try_acquire(1 + frames_delay_, boost::chrono::seconds(2)))
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
	const core::video_format_desc				output_format_desc_;
	const spl::shared_ptr<channel_consumer>		consumer_;
	core::constraints							pixel_constraints_;
	ffmpeg::frame_muxer							muxer_;

	std::queue<core::draw_frame>				frame_buffer_;

public:
	explicit channel_producer(
			const core::frame_producer_dependencies& dependecies,
			const spl::shared_ptr<core::video_channel>& channel,
			int frames_delay,
			bool no_auto_deinterlace)
		: frame_factory_(dependecies.frame_factory)
		, output_format_desc_(dependecies.format_desc)
		, consumer_(spl::make_shared<channel_consumer>(frames_delay))
		, muxer_(
				channel->video_format_desc().framerate,
				{ ffmpeg::create_input_pad(channel->video_format_desc(), channel->audio_channel_layout().num_channels) },
				dependecies.frame_factory,
				no_auto_deinterlace ? channel->video_format_desc() : get_progressive_format(channel->video_format_desc()),
				channel->audio_channel_layout(),
				L"",
				false,
				!no_auto_deinterlace)
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
		if (!muxer_.video_ready() || !muxer_.audio_ready())
		{
			auto read_frame = consumer_->receive();

			if (read_frame == core::const_frame::empty() || read_frame.image_data().empty())
				return core::draw_frame::late();

			auto video_frame = ffmpeg::create_frame();

			video_frame->data[0]				= const_cast<uint8_t*>(read_frame.image_data().begin());
			video_frame->linesize[0]			= static_cast<int>(read_frame.width()) * 4;
			video_frame->format				= AVPixelFormat::AV_PIX_FMT_BGRA;
			video_frame->width				= static_cast<int>(read_frame.width());
			video_frame->height				= static_cast<int>(read_frame.height());
			video_frame->interlaced_frame	= consumer_->get_video_format_desc().field_mode != core::field_mode::progressive;
			video_frame->top_field_first		= consumer_->get_video_format_desc().field_mode == core::field_mode::upper ? 1 : 0;
			video_frame->key_frame			= 1;

			muxer_.push(video_frame);
			muxer_.push(
					{
						std::make_shared<core::mutable_audio_buffer>(
								read_frame.audio_data().begin(),
								read_frame.audio_data().end())
					});
		}

		auto frame = muxer_.poll();

		if (frame == core::draw_frame::empty())
			return core::draw_frame::late();

		return frame;
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

	boost::rational<int> current_framerate() const
	{
		return muxer_.out_framerate();
	}
};

spl::shared_ptr<core::frame_producer> create_channel_producer(
		const core::frame_producer_dependencies& dependencies,
		const spl::shared_ptr<core::video_channel>& channel,
		int frames_delay,
		bool no_auto_deinterlace)
{
	auto producer = spl::make_shared<channel_producer>(dependencies, channel, frames_delay, no_auto_deinterlace);

	return core::create_framerate_producer(
			producer,
			[producer] { return producer->current_framerate(); },
			dependencies.format_desc.framerate,
			dependencies.format_desc.field_mode,
			dependencies.format_desc.audio_cadence);
}

}}
