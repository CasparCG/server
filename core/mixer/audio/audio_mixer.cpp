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

#include "../../stdafx.h"

#include "audio_mixer.h"

#include <core/mixer/write_frame.h>
#include <core/producer/frame/frame_transform.h>
#include <common/diagnostics/graph.h>

#include <tbb/cache_aligned_allocator.h>

#include <boost/range/adaptors.hpp>
#include <boost/range/distance.hpp>

#include <map>
#include <stack>
#include <vector>

namespace caspar { namespace core {

struct audio_item
{
	const void*			tag;
	frame_transform		transform;
	audio_buffer		audio_data;

	audio_item()
	{
	}

	audio_item(audio_item&& other)
		: tag(std::move(other.tag))
		, transform(std::move(other.transform))
		, audio_data(std::move(other.audio_data))
	{
	}
};

typedef std::vector<float, tbb::cache_aligned_allocator<float>> audio_buffer_ps;
	
struct audio_stream
{
	frame_transform		prev_transform;
	audio_buffer_ps		audio_data;
};

struct audio_mixer::implementation
{
	safe_ptr<diagnostics::graph>		graph_;
	std::stack<core::frame_transform>	transform_stack_;
	std::map<const void*, audio_stream>	audio_streams_;
	std::vector<audio_item>				items_;
	std::vector<size_t>					audio_cadence_;
	video_format_desc					format_desc_;
	float								master_volume_;
	float								previous_master_volume_;
	
public:
	implementation(const safe_ptr<diagnostics::graph>& graph)
		: graph_(graph)
		, format_desc_(video_format_desc::get(video_format::invalid))
		, master_volume_(1.0f)
		, previous_master_volume_(master_volume_)
	{
		graph_->set_color("volume", diagnostics::color(1.0f, 0.8f, 0.1f));
		transform_stack_.push(core::frame_transform());
	}
	
	void begin(core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_frame_transform());
	}

	void visit(core::write_frame& frame)
	{
		if(transform_stack_.top().volume < 0.002 || frame.audio_data().empty())
			return;

		audio_item item;
		item.tag		= frame.tag();
		item.transform	= transform_stack_.top();
		item.audio_data = std::move(frame.audio_data()); // Note: We don't need to care about upper/lower since audio_data is removed/moved from the last field.
		
		items_.push_back(std::move(item));		
	}

	void begin(const core::frame_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}
		
	void end()
	{
		transform_stack_.pop();
	}

	void set_master_volume(float volume)
	{
		master_volume_ = volume;
	}
	
	audio_buffer mix(const video_format_desc& format_desc)
	{	
		if(format_desc_ != format_desc)
		{
			audio_streams_.clear();
			audio_cadence_ = format_desc.audio_cadence;
			format_desc_ = format_desc;
		}		
		
		std::map<const void*, audio_stream>	next_audio_streams;

		BOOST_FOREACH(auto& item, items_)
		{			
			audio_buffer_ps next_audio;

			auto next_transform = item.transform;
			auto prev_transform = next_transform;

			const auto it = audio_streams_.find(item.tag);
			if(it != audio_streams_.end())
			{	
				prev_transform	= it->second.prev_transform;
				next_audio		= std::move(it->second.audio_data);
			}

			if(prev_transform.volume < 0.001 && next_transform.volume < 0.001)
				continue;
			
			const float prev_volume = static_cast<float>(prev_transform.volume) * previous_master_volume_;
			const float next_volume = static_cast<float>(next_transform.volume) * master_volume_;
									
			auto alpha = (next_volume-prev_volume)/static_cast<float>(item.audio_data.size()/format_desc.audio_channels);
			
			for(size_t n = 0; n < item.audio_data.size(); ++n)
			{
				auto sample_multiplier = (prev_volume + (n/format_desc_.audio_channels) * alpha);
				next_audio.push_back(item.audio_data[n] * sample_multiplier);
			}
										
			next_audio_streams[item.tag].prev_transform  = std::move(next_transform); // Store all active tags, inactive tags will be removed at the end.
			next_audio_streams[item.tag].audio_data		 = std::move(next_audio);			
		}

		previous_master_volume_ = master_volume_;
		items_.clear();

		audio_streams_ = std::move(next_audio_streams);
		
		if(audio_streams_.empty())		
			audio_streams_[nullptr].audio_data = audio_buffer_ps(audio_cadence_.front(), 0.0f);
				
		{ // sanity check

			auto nb_invalid_streams = boost::count_if(audio_streams_ | boost::adaptors::map_values, [&](const audio_stream& x)
			{
				return x.audio_data.size() < audio_cadence_.front();
			});

			if(nb_invalid_streams > 0)		
				CASPAR_LOG(trace) << "[audio_mixer] Incorrect frame audio cadence detected.";			
		}

		std::vector<float> result_ps(audio_cadence_.front(), 0.0f);

		BOOST_FOREACH(auto& stream, audio_streams_ | boost::adaptors::map_values)
		{
			//CASPAR_LOG(debug) << stream.audio_data.size() << L" : " << result_ps.size();

			if(stream.audio_data.size() < result_ps.size())
			{
				stream.audio_data.resize(result_ps.size(), 0.0f);
				CASPAR_LOG(trace) << L"[audio_mixer] Appended zero samples";
			}

			auto out = boost::range::transform(result_ps, stream.audio_data, std::begin(result_ps), std::plus<float>());
			stream.audio_data.erase(std::begin(stream.audio_data), std::begin(stream.audio_data) + std::distance(std::begin(result_ps), out));
		}
		
		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		
		audio_buffer result;
		result.reserve(result_ps.size());
		boost::range::transform(result_ps, std::back_inserter(result), [](float sample){return static_cast<int32_t>(sample);});		

		auto max = boost::range::max_element(result);

		graph_->set_value("volume", static_cast<double>(std::abs(*max))/std::numeric_limits<int32_t>::max());

		return result;
	}
};

audio_mixer::audio_mixer(const safe_ptr<diagnostics::graph>& graph) : impl_(new implementation(graph)){}
void audio_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void audio_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void audio_mixer::end(){impl_->end();}
void audio_mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
audio_buffer audio_mixer::operator()(const video_format_desc& format_desc){return impl_->mix(format_desc);}

}}