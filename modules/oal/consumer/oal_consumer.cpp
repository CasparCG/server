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

#include "oal_consumer.h"

#include <common/except.h>
#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/log.h>
#include <common/utf.h>
#include <common/env.h>
#include <common/future.h>
#include <common/param.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>
#include <core/mixer/audio/audio_util.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/video_format.h>
#include <core/help/help_sink.h>
#include <core/help/help_repository.h>

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>
#include <boost/thread/once.hpp>
#include <boost/algorithm/string.hpp>

#include <tbb/concurrent_queue.h>

#include <AL/alc.h>
#include <AL/al.h>

namespace caspar { namespace oal {

typedef cache_aligned_vector<int16_t> audio_buffer_16;

class device
{
	ALCdevice*		device_		= nullptr;
	ALCcontext*		context_	= nullptr;

public:
	device()
	{
		device_ = alcOpenDevice(nullptr);

		if(!device_)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to initialize audio device."));

		context_ = alcCreateContext(device_, nullptr);

		if(!context_)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to create audio context."));
			
		if(alcMakeContextCurrent(context_) == ALC_FALSE)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to activate audio context."));
	}

	~device()
	{
		alcMakeContextCurrent(nullptr);

		if(context_)
			alcDestroyContext(context_);

		if(device_)
			alcCloseDevice(device_);
	}

	ALCdevice* get()
	{
		return device_;
	}
};

void init_device()
{
	static std::unique_ptr<device> instance;
	static boost::once_flag f = BOOST_ONCE_INIT;
	
	boost::call_once(f, []{instance.reset(new device());});
}

struct oal_consumer : public core::frame_consumer
{
	core::monitor::subject							monitor_subject_;

	spl::shared_ptr<diagnostics::graph>				graph_;
	boost::timer									perf_timer_;
	tbb::atomic<int64_t>							presentation_age_;
	int												channel_index_		= -1;
	
	core::video_format_desc							format_desc_;
	core::audio_channel_layout						out_channel_layout_;
	std::unique_ptr<core::audio_channel_remapper>	channel_remapper_;

	ALuint											source_				= 0;
	std::vector<ALuint>								buffers_;
	int												latency_millis_;

	executor										executor_			{ L"oal_consumer" };

public:
	oal_consumer(const core::audio_channel_layout& out_channel_layout, int latency_millis)
		: out_channel_layout_(out_channel_layout)
		, latency_millis_(latency_millis)
	{
		presentation_age_ = 0;

		init_device();

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		diagnostics::register_graph(graph_);
	}

	~oal_consumer()
	{
		executor_.invoke([=]
		{		
			if(source_)
			{
				alSourceStop(source_);
				alDeleteSources(1, &source_);
			}

			for (auto& buffer : buffers_)
			{
				if(buffer)
					alDeleteBuffers(1, &buffer);
			};
		});
	}

	// frame consumer

	void initialize(const core::video_format_desc& format_desc, const core::audio_channel_layout& channel_layout, int channel_index) override
	{
		format_desc_	= format_desc;		
		channel_index_	= channel_index;
		if (out_channel_layout_ == core::audio_channel_layout::invalid())
			out_channel_layout_ = channel_layout.num_channels == 2 ? channel_layout : *core::audio_channel_layout_repository::get_default()->get_layout(L"stereo");

		out_channel_layout_.num_channels = 2;

		channel_remapper_.reset(new core::audio_channel_remapper(channel_layout, out_channel_layout_));
		graph_->set_text(print());

		executor_.begin_invoke([=]
		{		
			buffers_.resize(format_desc_.fps > 30 ? 8 : 4);
			alGenBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
			alGenSources(1, &source_);

			for(std::size_t n = 0; n < buffers_.size(); ++n)
			{
				audio_buffer_16 audio(format_desc_.audio_cadence[n % format_desc_.audio_cadence.size()]*2, 0);
				alBufferData(buffers_[n], AL_FORMAT_STEREO16, audio.data(), static_cast<ALsizei>(audio.size()*sizeof(int16_t)), format_desc_.audio_sample_rate);
				alSourceQueueBuffers(source_, 1, &buffers_[n]);
			}
			
			alSourcei(source_, AL_LOOPING, AL_FALSE);

			alSourcePlay(source_);	
		});
	}

	int64_t presentation_frame_age_millis() const override
	{
		return presentation_age_;
	}

	std::future<bool> send(core::const_frame frame) override
	{
		// Will only block if the default executor queue capacity of 512 is
		// exhausted, which should not happen
		executor_.begin_invoke([=]
		{
			ALenum state; 
			alGetSourcei(source_, AL_SOURCE_STATE,&state);
			if(state != AL_PLAYING)
			{
				for(int n = 0; n < buffers_.size()-1; ++n)
				{					
					ALuint buffer = 0;  
					alSourceUnqueueBuffers(source_, 1, &buffer);
					if(buffer)
					{
						std::vector<int16_t> audio(format_desc_.audio_cadence[n % format_desc_.audio_cadence.size()] * 2, 0);
						alBufferData(buffer, AL_FORMAT_STEREO16, audio.data(), static_cast<ALsizei>(audio.size()*sizeof(int16_t)), format_desc_.audio_sample_rate);
						alSourceQueueBuffers(source_, 1, &buffer);
					}
				}
				alSourcePlay(source_);		
				graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
			}

			auto audio = core::audio_32_to_16(channel_remapper_->mix_and_rearrange(frame.audio_data()));
			
			ALuint buffer = 0;  
			alSourceUnqueueBuffers(source_, 1, &buffer);
			if(buffer)
			{
				alBufferData(buffer, AL_FORMAT_STEREO16, audio.data(), static_cast<ALsizei>(audio.size()*sizeof(int16_t)), format_desc_.audio_sample_rate);
				alSourceQueueBuffers(source_, 1, &buffer);
			}
			else
				graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");

			graph_->set_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);		
			perf_timer_.restart();
			presentation_age_ = frame.get_age_millis() + latency_millis();
		});

		return make_ready_future(true);
	}
	
	std::wstring print() const override
	{
		return L"oal[" + boost::lexical_cast<std::wstring>(channel_index_) + L"|" + format_desc_.name + L"]";
	}

	std::wstring name() const override
	{
		return L"system-audio";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"system-audio");
		return info;
	}
	
	bool has_synchronization_clock() const override
	{
		return false;
	}

	int latency_millis() const
	{
		return latency_millis_;
	}
	
	int buffer_depth() const override
	{
		int delay_in_frames = static_cast<int>(latency_millis() / (1000.0 / format_desc_.fps));
		
		return delay_in_frames;
	}
		
	int index() const override
	{
		return 500;
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

void describe_consumer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"A system audio consumer.");
	sink.syntax(L"AUDIO {CHANNEL_LAYOUT [channel_layout:string]} {LATENCY [latency_millis:int|200]}");
	sink.para()->text(L"Uses the system's default audio playback device.");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 AUDIO");
	sink.example(L">> ADD 1 AUDIO CHANNEL_LAYOUT matrix", L"Uses the matrix channel layout");
	sink.example(L">> ADD 1 AUDIO LATENCY 500", L"Specifies that the system-audio chain: openal => driver => sound card => speaker output is 500ms");
}

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params, core::interaction_sink*)
{
	if(params.size() < 1 || !boost::iequals(params.at(0), L"AUDIO"))
		return core::frame_consumer::empty();

	auto channel_layout			= core::audio_channel_layout::invalid();
	auto channel_layout_spec	= get_param(L"CHANNEL_LAYOUT", params);

	if (!channel_layout_spec.empty())
	{
		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(channel_layout_spec);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + channel_layout_spec + L" not found."));

		channel_layout = *found_layout;
	}

	auto latency_millis			= get_param(L"LATENCY", params, 200);

	return spl::make_shared<oal_consumer>(channel_layout, latency_millis);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(const boost::property_tree::wptree& ptree, core::interaction_sink*)
{
	auto channel_layout			= core::audio_channel_layout::invalid();
	auto channel_layout_spec	= ptree.get_optional<std::wstring>(L"channel-layout");

	if (channel_layout_spec)
	{
		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(*channel_layout_spec);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + *channel_layout_spec + L" not found."));

		channel_layout = *found_layout;
	}

	auto latency_millis			= ptree.get(L"latency", 200);

	return spl::make_shared<oal_consumer>(channel_layout, latency_millis);
}

}}
