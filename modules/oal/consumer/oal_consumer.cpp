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

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/mixer/audio/audio_util.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/video_format.h>

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>

#include <tbb/concurrent_queue.h>

#include <al/alc.h>
#include <al/al.h>

#include <array>

namespace caspar { namespace oal {

typedef std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_buffer_16;

struct oal_consumer : public core::frame_consumer
{
	spl::shared_ptr<diagnostics::graph>					graph_;
	boost::timer										perf_timer_;
	int													channel_index_;
	
	core::video_format_desc								format_desc_;

	ALCdevice*											device_;
	ALCcontext*											context_;
	ALuint												source_;
	std::array<ALuint, 3>								buffers_;

	executor											executor_;

public:
	oal_consumer() 
		: channel_index_(-1)
		, device_(0)
		, context_(0)
		, source_(0)
		, executor_(L"oal_consumer")
	{
		buffers_.assign(0);

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		diagnostics::register_graph(graph_);
		
		executor_.invoke([=]
		{		
			device_ = alcOpenDevice(nullptr);

			if(!device_)
				BOOST_THROW_EXCEPTION(invalid_operation());

			context_ = alcCreateContext(device_, nullptr);

			if(!context_)
				BOOST_THROW_EXCEPTION(invalid_operation());
			
			if(alcMakeContextCurrent(context_) == ALC_FALSE)
				BOOST_THROW_EXCEPTION(invalid_operation());
		});
	}

	~oal_consumer()
	{
		executor_.begin_invoke([=]
		{		
			if(source_)
			{
				alSourceStop(source_);
				alDeleteSources(1, &source_);
			}

			BOOST_FOREACH(auto& buffer, buffers_)
			{
				if(buffer)
					alDeleteBuffers(1, &buffer);
			};

			alcMakeContextCurrent(nullptr);

			if(context_)
				alcDestroyContext(context_);

			if(device_)
				alcCloseDevice(device_);
		});
	}

	// frame consumer

	void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		format_desc_	= format_desc;		
		channel_index_	= channel_index;
		graph_->set_text(print());
		
		executor_.begin_invoke([=]
		{		
			alGenBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
			alGenSources(1, &source_);

			for(std::size_t n = 0; n < buffers_.size(); ++n)
			{
				std::vector<int16_t> audio(format_desc_.audio_cadence[n % format_desc_.audio_cadence.size()], 0);
				alBufferData(buffers_[n], AL_FORMAT_STEREO16, audio.data(), static_cast<ALsizei>(audio.size()*sizeof(int16_t)), format_desc_.audio_sample_rate);
				alSourceQueueBuffers(source_, 1, &buffers_[n]);
			}
			
			alSourcei(source_, AL_LOOPING, AL_FALSE);
		});
	}
	
	bool send(core::const_frame frame) override
	{			
		executor_.begin_invoke([=]
		{
			ALenum state; 
			alGetSourcei(source_, AL_SOURCE_STATE,&state);
			if(state != AL_PLAYING)
				alSourcePlay(source_);			

			auto audio = core::audio_32_to_16(frame.audio_data());
			
			ALuint buffer = 0;  
			alSourceUnqueueBuffers(source_, 1, &buffer);
			if(buffer)
			{
				alBufferData(buffer, AL_FORMAT_STEREO16, audio.data(), static_cast<ALsizei>(audio.size()*sizeof(int16_t)), format_desc_.audio_sample_rate);
				alSourceQueueBuffers(source_, 1, &buffer);
			}
			else
				graph_->set_tag("dropped-frame");

			graph_->set_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);		
			perf_timer_.restart();
		});

		return true;
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
	
	int buffer_depth() const override
	{
		return 3;
	}
		
	int index() const override
	{
		return 500;
	}
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"AUDIO")
		return core::frame_consumer::empty();

	return spl::make_shared<oal_consumer>();
}

spl::shared_ptr<core::frame_consumer> create_consumer()
{
	return spl::make_shared<oal_consumer>();
}

}}
