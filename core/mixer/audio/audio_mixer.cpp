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

#include <core/frame/write_frame.h>
#include <core/frame/frame_transform.h>
#include <common/diagnostics/graph.h>

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

struct audio_mixer::impl : boost::noncopyable
{
	std::stack<core::frame_transform>	transform_stack_;
	std::map<const void*, audio_stream>	audio_streams_;
	std::vector<audio_item>				items_;
	std::vector<int>					audio_cadence_;
	video_format_desc					format_desc_;
	
public:
	impl()
	{
		transform_stack_.push(core::frame_transform());
	}
	
	void push(frame_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}

	void visit(data_frame& frame)
	{
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
		
	void pop()
	{
		transform_stack_.pop();
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
			
			// Skip it if there is no existing audio stream and item has no audio-data.
			if(it == audio_streams_.end() && item.audio_data.empty()) 
				continue;
						
			const float prev_volume = static_cast<float>(prev_transform.volume);
			const float next_volume = static_cast<float>(next_transform.volume);
						
			// TODO: Move volume mixing into code below, in order to support audio sample counts not corresponding to frame audio samples.
			auto alpha = (next_volume-prev_volume)/static_cast<float>(item.audio_data.size()/format_desc.audio_channels);
			
			for(size_t n = 0; n < item.audio_data.size(); ++n)
				next_audio.push_back(item.audio_data[n] * (prev_volume + (n/format_desc_.audio_channels) * alpha));
										
			next_audio_streams[item.tag].prev_transform  = std::move(next_transform); // Store all active tags, inactive tags will be removed at the end.
			next_audio_streams[item.tag].audio_data		 = std::move(next_audio);			
		}				

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
		
		return result;
	}
};

audio_mixer::audio_mixer() : impl_(new impl()){}
void audio_mixer::push(frame_transform& transform){impl_->push(transform);}
void audio_mixer::visit(data_frame& frame){impl_->visit(frame);}
void audio_mixer::pop(){impl_->pop();}
audio_buffer audio_mixer::operator()(const video_format_desc& format_desc){return impl_->mix(format_desc);}

}}