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

#include <common/exception/exceptions.h>
#include <common/diagnostics/graph.h>
#include <common/log/log.h>
#include <common/utility/timer.h>
#include <common/utility/string.h>
#include <common/concurrency/future_util.h>

#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>
#include <core/video_format.h>

#include <core/mixer/read_frame.h>

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>
#include <boost/thread/future.hpp>
#include <boost/optional.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace oal {

typedef std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_buffer_16;

struct oal_consumer : public core::frame_consumer,  public sf::SoundStream
{
	safe_ptr<diagnostics::graph>						graph_;
	boost::timer										perf_timer_;
	int													channel_index_;

	tbb::concurrent_bounded_queue<std::shared_ptr<audio_buffer_16>>	input_;
	boost::circular_buffer<audio_buffer_16>				container_;
	tbb::atomic<bool>									is_running_;
	core::audio_buffer									temp;

	core::video_format_desc								format_desc_;
public:
	oal_consumer() 
		: container_(16)
		, channel_index_(-1)
	{
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		diagnostics::register_graph(graph_);

		is_running_ = true;
		input_.set_capacity(1);
	}

	~oal_consumer()
	{
		is_running_ = false;
		input_.try_push(std::make_shared<audio_buffer_16>());
		input_.try_push(std::make_shared<audio_buffer_16>());
		Stop();
		input_.try_push(std::make_shared<audio_buffer_16>());
		input_.try_push(std::make_shared<audio_buffer_16>());

		CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";	
	}

	// frame consumer

	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		format_desc_	= format_desc;		
		channel_index_	= channel_index;
		graph_->set_text(print());

		if(Status() != Playing)
		{
			sf::SoundStream::Initialize(2, 48000);
			Play();		
		}
		CASPAR_LOG(info) << print() << " Sucessfully Initialized.";
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		auto buffer = std::make_shared<audio_buffer_16>(core::audio_32_to_16(frame->audio_data()));

		if (!input_.try_push(buffer))
			graph_->set_tag("dropped-frame");


		return wrap_as_future(is_running_.load());
	}
	
	virtual std::wstring print() const override
	{
		return L"oal[" + boost::lexical_cast<std::wstring>(channel_index_) + L"|" + format_desc_.name + L"]";
	}

	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"oal-consumer");
		return info;
	}
	
	virtual size_t buffer_depth() const override
	{
		return 6;
	}

	// oal_consumer
	
	virtual bool OnGetData(sf::SoundStream::Chunk& data) override
	{		
		std::shared_ptr<audio_buffer_16> audio_data;		

		if (!input_.try_pop(audio_data))
		{
			graph_->set_tag("late-frame");
			input_.pop(audio_data); // Block until available
		}

		container_.push_back(std::move(*audio_data));
		data.Samples = container_.back().data();
		data.NbSamples = container_.back().size();	
		
		graph_->set_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);		
		perf_timer_.restart();

		return is_running_;
	}

	virtual int index() const override
	{
		return 500;
	}
};

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"AUDIO")
		return core::frame_consumer::empty();

	return make_safe<oal_consumer>();
}

safe_ptr<core::frame_consumer> create_consumer()
{
	return make_safe<oal_consumer>();
}

}}
