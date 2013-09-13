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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "portaudio_consumer.h"

#include <cmath>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/timer.hpp>

#include <portaudio.h>

#include <common/log/log.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>
#include <common/concurrency/future_util.h>
#include <common/diagnostics/graph.h>

#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>
#include <core/video_format.h>
#include <core/parameters/parameters.h>
#include <core/mixer/read_frame.h>

namespace caspar { namespace portaudio {

#define PA_CHECK(err) if (err != paNoError) \
	BOOST_THROW_EXCEPTION(caspar_exception() \
			<< msg_info(std::string("PortAudio error: ") \
			+ Pa_GetErrorText(err)))

typedef std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_buffer_16;

int callback(
		const void*, // input
		void* output,
		unsigned long sample_frames_per_buffer,
		const PaStreamCallbackTimeInfo* time_info,
		PaStreamCallbackFlags status_flags,
		void* user_data);

struct portaudio_initializer
{
	portaudio_initializer()
	{
		PA_CHECK(Pa_Initialize());
	}

	~portaudio_initializer()
	{
		try
		{
			PA_CHECK(Pa_Terminate());
		}
		catch (...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
};

struct portaudio_consumer : public core::frame_consumer
{
	safe_ptr<diagnostics::graph>									graph_;
	boost::timer													tick_timer_;
	int																channel_index_;

	tbb::atomic<bool>												started_;
	tbb::concurrent_bounded_queue<std::shared_ptr<audio_buffer_16>>	frames_in_queue_;
	std::pair<std::shared_ptr<audio_buffer_16>, unsigned int>		frame_being_consumed_;
	std::shared_ptr<audio_buffer_16>								to_deallocate_in_output_thread_;

	core::video_format_desc											format_desc_;
	core::channel_layout											channel_layout_;

	std::shared_ptr<PaStream>										stream_;
public:
	portaudio_consumer()
		: channel_index_(-1)
		, channel_layout_(
				core::default_channel_layout_repository().get_by_name(L"STEREO"))
	{
		started_ = false;
		frames_in_queue_.set_capacity(2);

		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));

		diagnostics::register_graph(graph_);
	}

	bool promote_frame()
	{
		to_deallocate_in_output_thread_.swap(frame_being_consumed_.first);
		frame_being_consumed_.first.reset();
		frame_being_consumed_.second = 0;

		return frames_in_queue_.try_pop(frame_being_consumed_.first);
	}

	void write_samples(
			int16_t* output,
			unsigned int sample_frames_per_buffer,
			PaStreamCallbackFlags status_flags)
	{
		if (!frame_being_consumed_.first)
		{
			promote_frame();
		}

		auto needed = sample_frames_per_buffer * channel_layout_.num_channels;

		while (needed > 0 && frame_being_consumed_.first)
		{
			auto start_index = frame_being_consumed_.second;
			auto available = frame_being_consumed_.first->size() - start_index;
			auto to_provide = std::min(needed, available);

			std::memcpy(
					output,
					frame_being_consumed_.first->data() + start_index,
					to_provide * sizeof(int16_t));

			output += to_provide;
			needed -= to_provide;
			available -= to_provide;
			frame_being_consumed_.second += to_provide;

			if (available == 0 && !promote_frame())
				break;
		}

		if (needed > 0)
		{
			std::memset(output, 0, needed * sizeof(int16_t));

			CASPAR_LOG(trace) << print() << L"late-frame: Inserted "
					<< needed << L" zero-samples";
			graph_->set_tag("late-frame");
		}
	}

	~portaudio_consumer()
	{
		if (stream_ && started_)
			PA_CHECK(Pa_StopStream(stream_.get()));

		CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";	
	}

	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		format_desc_	= format_desc;		
		channel_index_	= channel_index;
		graph_->set_text(print());

		static portaudio_initializer init;

		auto device_index = Pa_GetDefaultOutputDevice();

		if (device_index == paNoDevice)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No port audio device detected"));

		auto device = Pa_GetDeviceInfo(device_index);

		PaStreamParameters parameters;
		parameters.channelCount = channel_layout_.num_channels;
		parameters.device = device_index;
		parameters.hostApiSpecificStreamInfo = nullptr;
		parameters.sampleFormat = paInt16;
		parameters.suggestedLatency = device->defaultLowOutputLatency;

		PaStream* stream;

		PA_CHECK(Pa_OpenStream(
				&stream,
				nullptr, // input config
				&parameters,
				format_desc.audio_sample_rate,
				*std::min_element(format_desc.audio_cadence.begin(), format_desc.audio_cadence.end()),
				0,
				callback,
				this));
		stream_.reset(stream, [](PaStream* s) { Pa_CloseStream(s); });

		CASPAR_LOG(info) << print() << " Sucessfully Initialized.";
	}

	virtual int64_t presentation_frame_age_millis() const override
	{
		auto info = Pa_GetStreamInfo(stream_.get());

		if (info == nullptr)
			return 0;

		return static_cast<int64_t>(info->outputLatency * 1000.0);
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		to_deallocate_in_output_thread_.reset();
		std::shared_ptr<audio_buffer_16> buffer;

		if (core::needs_rearranging(
				frame->multichannel_view(),
				channel_layout_,
				channel_layout_.num_channels))
		{
			core::audio_buffer downmixed;
			downmixed.resize(
					frame->multichannel_view().num_samples() 
							* channel_layout_.num_channels,
					0);

			auto dest_view = core::make_multichannel_view<int32_t>(
					downmixed.begin(), downmixed.end(), channel_layout_);

			core::rearrange_or_rearrange_and_mix(
					frame->multichannel_view(),
					dest_view,
					core::default_mix_config_repository());

			buffer = std::make_shared<audio_buffer_16>(
					core::audio_32_to_16(downmixed));
		}
		else
		{
			buffer = std::make_shared<audio_buffer_16>(
					core::audio_32_to_16(frame->audio_data()));
		}

		if (!frames_in_queue_.try_push(buffer))
			graph_->set_tag("dropped-frame");

		if (!started_)
		{
			PA_CHECK(Pa_StartStream(stream_.get()));
			started_ = true;
		}

		return wrap_as_future(true);
	}
	
	virtual std::wstring print() const override
	{
		return L"portaudio[" + boost::lexical_cast<std::wstring>(channel_index_) + L"|" + format_desc_.name + L"]";
	}

	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"portaudio-consumer");
		return info;
	}
	
	virtual size_t buffer_depth() const override
	{
		return 3;
	}

	virtual int index() const override
	{
		return 530;
	}
};

int callback(
		const void*, // input
		void* output,
		unsigned long sample_frames_per_buffer,
		const PaStreamCallbackTimeInfo* time_info,
		PaStreamCallbackFlags status_flags,
		void* user_data)
{
	win32_exception::ensure_handler_installed_for_thread(
			"portaudio-callback-thread");
	auto consumer = static_cast<portaudio_consumer*>(user_data);

	consumer->write_samples(
			static_cast<int16_t*>(output),
			sample_frames_per_buffer,
			status_flags);

	return 0;
}

safe_ptr<core::frame_consumer> create_consumer(const core::parameters& params)
{
	if(params.size() < 1 || params[0] != L"AUDIO")
		return core::frame_consumer::empty();

	return make_safe<portaudio_consumer>();
}

safe_ptr<core::frame_consumer> create_consumer()
{
	return make_safe<portaudio_consumer>();
}

}}
