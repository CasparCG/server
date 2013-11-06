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

#include "oal_consumer.h"

#include <iterator>

#include <common/exception/exceptions.h>
#include <common/diagnostics/graph.h>
#include <common/log/log.h>
#include <common/utility/timer.h>
#include <common/utility/string.h>
#include <common/concurrency/future_util.h>
#include <common/exception/win32_exception.h>

#include <core/parameters/parameters.h>
#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>
#include <core/video_format.h>

#include <core/mixer/read_frame.h>

#include <SFML/Audio.hpp>

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
	boost::timer										tick_timer_;
	int													channel_index_;

	tbb::concurrent_bounded_queue<std::shared_ptr<std::vector<audio_buffer_16>>>	input_;
	std::shared_ptr<std::vector<audio_buffer_16>>		chunk_builder_;
	audio_buffer_16										container_;
	tbb::atomic<bool>									is_running_;

	core::video_format_desc								format_desc_;
	core::channel_layout								channel_layout_;
	size_t												frames_to_buffer_;
public:
	oal_consumer() 
		: channel_index_(-1)
		, channel_layout_(
				core::default_channel_layout_repository().get_by_name(
						L"STEREO"))
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
		input_.try_push(std::make_shared<std::vector<audio_buffer_16>>());
		input_.try_push(std::make_shared<std::vector<audio_buffer_16>>());
		Stop();
		input_.try_push(std::make_shared<std::vector<audio_buffer_16>>());
		input_.try_push(std::make_shared<std::vector<audio_buffer_16>>());

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
			sf::SoundStream::Initialize(2, format_desc.audio_sample_rate);

			// Each time OnGetData is called it seems to no longer be enough
			// with the samples of one frame (since the change to statically
			// linked SFML 1.6), so the samples of a few frames is needed to be
			// collected.
			static const double SAMPLES_NEEDED_BY_SFML_MULTIPLE = 0.1;
			int min_num_samples_in_chunk = static_cast<int>(format_desc.audio_sample_rate * SAMPLES_NEEDED_BY_SFML_MULTIPLE);
			int min_num_samples_in_frame = *std::min_element(format_desc.audio_cadence.begin(), format_desc.audio_cadence.end());
 			int min_frames_to_buffer = min_num_samples_in_chunk / min_num_samples_in_frame + (min_num_samples_in_chunk % min_num_samples_in_frame ? 1 : 0);
			frames_to_buffer_ = min_frames_to_buffer;

			Play();
		}
		CASPAR_LOG(info) << print() << " Sucessfully Initialized.";
	}

	virtual int64_t presentation_frame_age_millis() const override
	{
		return 0;
	}

	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		audio_buffer_16 buffer;

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

			buffer = core::audio_32_to_16(downmixed);
		}
		else
		{
			buffer = core::audio_32_to_16(frame->audio_data());
		}

		if (!chunk_builder_)
		{
			chunk_builder_.reset(new std::vector<audio_buffer_16>);
			chunk_builder_->push_back(std::move(buffer));
		}
		else
		{
			chunk_builder_->push_back(std::move(buffer));
		}

		if (chunk_builder_->size() == frames_to_buffer_)
		{
			if (!input_.try_push(chunk_builder_))
			{
				graph_->set_tag("dropped-frame");
				chunk_builder_->pop_back();
			}
			else
			{
				chunk_builder_.reset();
			}
		}

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
		win32_exception::ensure_handler_installed_for_thread(
				"sfml-audio-thread");
		std::shared_ptr<std::vector<audio_buffer_16>> audio_data;		

		if (!input_.try_pop(audio_data))
		{
			graph_->set_tag("late-frame");
			input_.pop(audio_data); // Block until available
		}

		container_.clear();
		
		// Concatenate to one large buffer.
		BOOST_FOREACH(auto& buffer, *audio_data)
		{
			std::copy(buffer.begin(), buffer.end(), std::back_inserter(container_));
		}

		data.Samples = container_.data();
		data.NbSamples = container_.size();	
		
		graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5 / frames_to_buffer_);		
		tick_timer_.restart();

		return is_running_;
	}

	virtual int index() const override
	{
		return 500;
	}
};

safe_ptr<core::frame_consumer> create_consumer(const core::parameters& params)
{
	if(params.size() < 1 || params[0] != L"OAL")
		return core::frame_consumer::empty();

	return make_safe<oal_consumer>();
}

safe_ptr<core::frame_consumer> create_consumer()
{
	return make_safe<oal_consumer>();
}

}}
