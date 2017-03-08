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

#include "../../StdAfx.h"

#include "audio_mixer.h"

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/audio_channel_layout.h>
#include <core/monitor/monitor.h>

#include <common/diagnostics/graph.h>
#include <common/linq.h>

#include <boost/range/adaptors.hpp>
#include <boost/range/distance.hpp>
#include <boost/lexical_cast.hpp>

#include <map>
#include <stack>
#include <vector>

namespace caspar { namespace core {

struct audio_item
{
	const void*				tag				= nullptr;
	audio_transform			transform;
	audio_buffer			audio_data;
	audio_channel_layout	channel_layout	= audio_channel_layout::invalid();

	audio_item()
	{
	}

	audio_item(audio_item&& other)
		: tag(std::move(other.tag))
		, transform(std::move(other.transform))
		, audio_data(std::move(other.audio_data))
		, channel_layout(std::move(other.channel_layout))
	{
	}
};

typedef cache_aligned_vector<double> audio_buffer_ps;

struct audio_stream
{
	audio_transform							prev_transform;
	audio_buffer_ps							audio_data;
	std::unique_ptr<audio_channel_remapper>	channel_remapper;
	bool									remapping_failed	= false;
	bool									is_still			= false;
};

struct audio_mixer::impl : boost::noncopyable
{
	monitor::subject					monitor_subject_		{ "/audio" };
	std::stack<core::audio_transform>	transform_stack_;
	std::map<const void*, audio_stream>	audio_streams_;
	std::vector<audio_item>				items_;
	std::vector<int>					audio_cadence_;
	video_format_desc					format_desc_;
	audio_channel_layout				channel_layout_			= audio_channel_layout::invalid();
	float								master_volume_			= 1.0f;
	float								previous_master_volume_	= master_volume_;
	spl::shared_ptr<diagnostics::graph>	graph_;
public:
	impl(spl::shared_ptr<diagnostics::graph> graph)
		: graph_(std::move(graph))
	{
		graph_->set_color("volume", diagnostics::color(1.0f, 0.8f, 0.1f));
		graph_->set_color("audio-clipping", diagnostics::color(0.3f, 0.6f, 0.3f));
		transform_stack_.push(core::audio_transform());
	}

	void push(const frame_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform.audio_transform);
	}

	void visit(const const_frame& frame)
	{
		if(transform_stack_.top().volume < 0.002 || frame.audio_data().empty())
			return;

		audio_item item;
		item.tag			= frame.stream_tag();
		item.transform		= transform_stack_.top();
		item.audio_data		= frame.audio_data();
		item.channel_layout = frame.audio_channel_layout();

		if(item.transform.is_still)
			item.transform.volume = 0.0;

		items_.push_back(std::move(item));
	}

	void begin(const core::audio_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}

	void pop()
	{
		transform_stack_.pop();
	}

	void set_master_volume(float volume)
	{
		master_volume_ = volume;
	}

	float get_master_volume()
	{
		return master_volume_;
	}

	audio_buffer mix(const video_format_desc& format_desc, const audio_channel_layout& channel_layout)
	{
		if(format_desc_ != format_desc || channel_layout_ != channel_layout)
		{
			audio_streams_.clear();
			audio_cadence_ = format_desc.audio_cadence;
			format_desc_ = format_desc;
			channel_layout_ = channel_layout;
		}

		std::map<const void*, audio_stream>	next_audio_streams;

		for (auto& item : items_)
		{
			audio_buffer_ps next_audio;
			std::unique_ptr<audio_channel_remapper> channel_remapper;
			bool remapping_failed = false;

			auto next_transform = item.transform;
			auto prev_transform = next_transform;

			auto tag = item.tag;

			auto it = next_audio_streams.find(tag);
			bool found = it != next_audio_streams.end();

			if (!found)
			{
				it = audio_streams_.find(tag);
				found = it != audio_streams_.end();
			}

			if (found)
			{
				prev_transform = it->second.prev_transform;
				next_audio = std::move(it->second.audio_data);
				channel_remapper = std::move(it->second.channel_remapper);
				remapping_failed = it->second.remapping_failed;
			}

			if (remapping_failed)
			{
				CASPAR_LOG(trace) << "[audio_mixer] audio channel remapping already failed for stream.";
				next_audio_streams[tag].remapping_failed = true;
				continue;
			}

			// Skip it if there is no existing audio stream and item has no audio-data.
			if(!found && item.audio_data.empty())
				continue;

			if (item.channel_layout == audio_channel_layout::invalid())
			{
				CASPAR_LOG(warning) << "[audio_mixer] invalid audio channel layout for item";
				next_audio_streams[tag].remapping_failed = true;
				continue;
			}

			if (!channel_remapper)
			{
				try
				{
					channel_remapper.reset(new audio_channel_remapper(item.channel_layout, channel_layout_));
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					CASPAR_LOG(error) << "[audio_mixer] audio channel remapping failed for stream.";
					next_audio_streams[tag].remapping_failed = true;
					continue;
				}
			}

			item.audio_data = channel_remapper->mix_and_rearrange(item.audio_data);

			const double prev_volume = prev_transform.volume * previous_master_volume_;
			const double next_volume = next_transform.volume * master_volume_;

			// TODO: Move volume mixing into code below, in order to support audio sample counts not corresponding to frame audio samples.
			auto alpha = (next_volume-prev_volume)/static_cast<double>(item.audio_data.size()/channel_layout_.num_channels);

			for(size_t n = 0; n < item.audio_data.size(); ++n)
			{
				auto sample_multiplier = (prev_volume + (n / channel_layout_.num_channels) * alpha);
				next_audio.push_back(item.audio_data.data()[n] * sample_multiplier);
			}

			next_audio_streams[tag].prev_transform		= std::move(next_transform); // Store all active tags, inactive tags will be removed at the end.
			next_audio_streams[tag].audio_data			= std::move(next_audio);
			next_audio_streams[tag].channel_remapper	= std::move(channel_remapper);
			next_audio_streams[tag].remapping_failed	= remapping_failed;
			next_audio_streams[tag].is_still			= item.transform.is_still;
		}

		previous_master_volume_ = master_volume_;
		items_.clear();

		audio_streams_ = std::move(next_audio_streams);

		if(audio_streams_.empty())
			audio_streams_[nullptr].audio_data = audio_buffer_ps(audio_size(audio_cadence_.front()), 0.0);

		{ // sanity check

			auto nb_invalid_streams = cpplinq::from(audio_streams_)
				.select(values())
				.where([&](const audio_stream& x)
				{
					return !x.remapping_failed && x.audio_data.size() < audio_size(audio_cadence_.front());
				})
				.count();

			if(nb_invalid_streams > 0)
				CASPAR_LOG(trace) << "[audio_mixer] Incorrect frame audio cadence detected.";
		}

		audio_buffer_ps result_ps(audio_size(audio_cadence_.front()), 0.0);
		for (auto& stream : audio_streams_ | boost::adaptors::map_values)
		{
			if (stream.audio_data.size() < result_ps.size())
			{
				auto samples = (result_ps.size() - stream.audio_data.size()) / channel_layout_.num_channels;
				CASPAR_LOG(trace) << L"[audio_mixer] Appended " << samples << L" zero samples";
				CASPAR_LOG(trace) << L"[audio_mixer] Actual number of samples " << stream.audio_data.size() / channel_layout_.num_channels;
				CASPAR_LOG(trace) << L"[audio_mixer] Wanted number of samples " << result_ps.size() / channel_layout_.num_channels;
				stream.audio_data.resize(result_ps.size(), 0.0);
			}

			auto out = boost::range::transform(result_ps, stream.audio_data, std::begin(result_ps), std::plus<double>());
			stream.audio_data.erase(std::begin(stream.audio_data), std::begin(stream.audio_data) + std::distance(std::begin(result_ps), out));
		}

		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);

		auto result_owner = spl::make_shared<mutable_audio_buffer>();
		auto& result = *result_owner;
		result.reserve(result_ps.size());
		const int32_t min_amplitude = std::numeric_limits<int32_t>::min();
		const int32_t max_amplitude = std::numeric_limits<int32_t>::max();
		bool clipping = false;
		boost::range::transform(result_ps, std::back_inserter(result), [&](double sample)
		{
			if (sample > max_amplitude)
			{
				clipping = true;
				return max_amplitude;
			}
			else if (sample < min_amplitude)
			{
				clipping = true;
				return min_amplitude;
			}
			else
				return static_cast<int32_t>(sample);
		});

		if (clipping)
			graph_->set_tag(diagnostics::tag_severity::WARNING, "audio-clipping");

		const int num_channels = channel_layout_.num_channels;
		monitor_subject_ << monitor::message("/nb_channels") % num_channels;

		auto max = std::vector<int32_t>(num_channels, std::numeric_limits<int32_t>::min());

		for (size_t n = 0; n < result.size(); n += num_channels)
			for (int ch = 0; ch < num_channels; ++ch)
				max[ch] = std::max(max[ch], std::abs(result[n + ch]));

		// Makes the dBFS of silence => -dynamic range of 32bit LPCM => about -192 dBFS
		// Otherwise it would be -infinity
		static const auto MIN_PFS = 0.5f / static_cast<float>(std::numeric_limits<int32_t>::max());

		for (int i = 0; i < num_channels; ++i)
		{
			const auto pFS = max[i] / static_cast<float>(std::numeric_limits<int32_t>::max());
			const auto dBFS = 20.0f * std::log10(std::max(MIN_PFS, pFS));

			auto chan_str = boost::lexical_cast<std::string>(i + 1);

			monitor_subject_ << monitor::message("/" + chan_str + "/pFS") % pFS;
			monitor_subject_ << monitor::message("/" + chan_str + "/dBFS") % dBFS;
		}

		graph_->set_value("volume", static_cast<double>(*boost::max_element(max)) / std::numeric_limits<int32_t>::max());

		return caspar::array<int32_t>(result.data(), result.size(), true, std::move(result_owner));
	}

	size_t audio_size(size_t num_samples) const
	{
		return num_samples * channel_layout_.num_channels;
	}
};

audio_mixer::audio_mixer(spl::shared_ptr<diagnostics::graph> graph) : impl_(new impl(std::move(graph))){}
void audio_mixer::push(const frame_transform& transform){impl_->push(transform);}
void audio_mixer::visit(const const_frame& frame){impl_->visit(frame);}
void audio_mixer::pop(){impl_->pop();}
void audio_mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
float audio_mixer::get_master_volume() { return impl_->get_master_volume(); }
audio_buffer audio_mixer::operator()(const video_format_desc& format_desc, const audio_channel_layout& channel_layout){ return impl_->mix(format_desc, channel_layout); }
monitor::subject& audio_mixer::monitor_output(){ return impl_->monitor_subject_; }

}}
