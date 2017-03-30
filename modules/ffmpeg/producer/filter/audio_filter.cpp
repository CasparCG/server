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

#include "../../StdAfx.h"

#include "audio_filter.h"

#include "../../ffmpeg_error.h"
#include "../../ffmpeg.h"
#include "../util/util.h"

#include <common/assert.h>
#include <common/except.h>

#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/rational.hpp>

#include <cstdio>
#include <sstream>
#include <string>
#include <algorithm>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
	#include <libavutil/avutil.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/opt.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/avcodec.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {

std::string create_sourcefilter_str(const audio_input_pad& input_pad, std::string name)
{
	const auto asrc_options = (boost::format("abuffer=time_base=%1%/%2%:sample_rate=%3%:sample_fmt=%4%:channel_layout=0x%|5$x| [%6%]")
		% input_pad.time_base.numerator() % input_pad.time_base.denominator()
		% input_pad.sample_rate
		% av_get_sample_fmt_name(input_pad.sample_fmt)
		% input_pad.audio_channel_layout
		% name).str();

	return asrc_options;
}

std::string create_sinkfilter_str(const audio_output_pad& output_pad, std::string name)
{
	const auto asink_options = (boost::format("[%1%] abuffersink") % name).str();

	return asink_options;
}

struct audio_filter::implementation
{
	std::string						filtergraph_;

	std::shared_ptr<AVFilterGraph>	audio_graph_;
	std::vector<AVFilterContext*>	audio_graph_inputs_;
	std::vector<AVFilterContext*>	audio_graph_outputs_;

	std::vector<audio_input_pad>	input_pads_;

	implementation(
			std::vector<audio_input_pad> input_pads,
			std::vector<audio_output_pad> output_pads,
			const std::string& filtergraph)
		: filtergraph_(boost::to_lower_copy(filtergraph))
		, input_pads_(std::move(input_pads))
	{
		if (input_pads_.empty())
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("input_pads cannot be empty"));

		if (output_pads.empty())
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("output_pads cannot be empty"));

		audio_graph_.reset(
			avfilter_graph_alloc(),
			[](AVFilterGraph* p)
		{
			avfilter_graph_free(&p);
		});

		std::vector<std::string> complete_filter_graph;

		{
			int i = 0;
			for (auto& input_pad : input_pads_)
				complete_filter_graph.push_back(create_sourcefilter_str(input_pad, "a:" + boost::lexical_cast<std::string>(i++)));
		}

		if (filtergraph_.empty())
			complete_filter_graph.push_back("[a:0] anull [aout:0]");
		else
			complete_filter_graph.push_back(filtergraph_);

		{
			int i = 0;
			for (auto& output_pad : output_pads)
			{
				complete_filter_graph.push_back(create_sinkfilter_str(output_pad, "aout:" + boost::lexical_cast<std::string>(i++)));

				output_pad.sample_fmts.push_back(AVSampleFormat::AV_SAMPLE_FMT_NONE);
				output_pad.audio_channel_layouts.push_back(0);
				output_pad.sample_rates.push_back(-1);
			}
		}

		configure_filtergraph(
				*audio_graph_,
				boost::join(complete_filter_graph, ";"),
				audio_graph_inputs_,
				audio_graph_outputs_,
				output_pads);

		if (is_logging_quiet_for_thread())
			CASPAR_LOG(trace)
				<< 	u16(std::string("\n")
					+ avfilter_graph_dump(
							audio_graph_.get(),
							nullptr));
		else
			CASPAR_LOG(debug)
				<< u16(std::string("\n")
					+ avfilter_graph_dump(
						audio_graph_.get(),
						nullptr));
	}

	void configure_filtergraph(
			AVFilterGraph& graph,
			const std::string& filtergraph,
			std::vector<AVFilterContext*>& source_contexts,
			std::vector<AVFilterContext*>& sink_contexts,
			const std::vector<audio_output_pad>& output_pads)
	{
		AVFilterInOut* outputs	= nullptr;
		AVFilterInOut* inputs	= nullptr;

		FF(avfilter_graph_parse2(
				&graph,
				filtergraph.c_str(),
				&inputs,
				&outputs));

		// Workaround because outputs and inputs are not filled in for some reason
		for (unsigned i = 0; i < graph.nb_filters; ++i)
		{
			auto filter = graph.filters[i];

			if (std::string(filter->filter->name) == "abuffer")
				source_contexts.push_back(filter);

			if (std::string(filter->filter->name) == "abuffersink")
				sink_contexts.push_back(filter);
		}

		for (AVFilterInOut* iter = inputs; iter; iter = iter->next)
			source_contexts.push_back(iter->filter_ctx);

		for (AVFilterInOut* iter = outputs; iter; iter = iter->next)
			sink_contexts.push_back(iter->filter_ctx);

		for (int i = 0; i < sink_contexts.size(); ++i)
		{
			auto sink_context = sink_contexts.at(i);
			auto& output_pad = output_pads.at(i);

#pragma warning (push)
#pragma warning (disable : 4245)
			FF(av_opt_set_int_list(
				sink_context,
				"sample_fmts",
				output_pad.sample_fmts.data(),
				-1,
				AV_OPT_SEARCH_CHILDREN));

			FF(av_opt_set_int_list(
				sink_context,
				"channel_layouts",
				output_pad.audio_channel_layouts.data(),
				0,
				AV_OPT_SEARCH_CHILDREN));

			FF(av_opt_set_int_list(
				sink_context,
				"sample_rates",
				output_pad.sample_rates.data(),
				-1,
				AV_OPT_SEARCH_CHILDREN));
#pragma warning (pop)
		}

		FF(avfilter_graph_config(
			&graph,
			nullptr));
	}

	void set_guaranteed_output_num_samples_per_frame(int output_pad_id, int num_samples)
	{
		av_buffersink_set_frame_size(audio_graph_outputs_.at(output_pad_id), num_samples);
	}

	void push(int input_pad_id, const std::shared_ptr<AVFrame>& src_av_frame)
	{
		FF(av_buffersrc_add_frame(
			audio_graph_inputs_.at(input_pad_id),
			src_av_frame.get()));
	}

	void push(int input_pad_id, const boost::iterator_range<const int32_t*>& frame_samples)
	{
		auto& input_pad				= input_pads_.at(input_pad_id);
		auto num_samples			= frame_samples.size() / av_get_channel_layout_nb_channels(input_pad.audio_channel_layout);
		auto input_frame			= ffmpeg::create_frame();

		input_frame->channels		= av_get_channel_layout_nb_channels(input_pad.audio_channel_layout);
		input_frame->channel_layout	= input_pad.audio_channel_layout;
		input_frame->sample_rate		= input_pad.sample_rate;
		input_frame->nb_samples		= static_cast<int>(num_samples);
		input_frame->format			= input_pad.sample_fmt;
		input_frame->pts				= 0;

		av_samples_fill_arrays(
				input_frame->extended_data,
				input_frame->linesize,
				reinterpret_cast<const std::uint8_t*>(frame_samples.begin()),
				input_frame->channels,
				input_frame->nb_samples,
				static_cast<AVSampleFormat>(input_frame->format),
				16);

		push(input_pad_id, input_frame);
	}

	std::shared_ptr<AVFrame> poll(int output_pad_id)
	{
		auto filt_frame = create_frame();

		const auto ret = av_buffersink_get_frame(
			audio_graph_outputs_.at(output_pad_id),
			filt_frame.get());

		if(ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
			return nullptr;

		FF_RET(ret, "poll");

		return filt_frame;
	}

	const AVFilterLink& get_output_pad_info(int output_pad_id) const
	{
		return *audio_graph_outputs_.at(output_pad_id)->inputs[0];
	}
};

audio_filter::audio_filter(
		std::vector<audio_input_pad> input_pads,
		std::vector<audio_output_pad> output_pads,
		const std::string& filtergraph)
	: impl_(new implementation(std::move(input_pads), std::move(output_pads), filtergraph))
{
}
audio_filter::audio_filter(audio_filter&& other) : impl_(std::move(other.impl_)){}
audio_filter& audio_filter::operator=(audio_filter&& other){impl_ = std::move(other.impl_); return *this;}
void audio_filter::set_guaranteed_output_num_samples_per_frame(int output_pad_id, int num_samples) { impl_->set_guaranteed_output_num_samples_per_frame(output_pad_id, num_samples); }
void audio_filter::push(int input_pad_id, const std::shared_ptr<AVFrame>& frame){impl_->push(input_pad_id, frame);}
void audio_filter::push(int input_pad_id, const boost::iterator_range<const int32_t*>& frame_samples) { impl_->push(input_pad_id, frame_samples); }
std::shared_ptr<AVFrame> audio_filter::poll(int output_pad_id){return impl_->poll(output_pad_id);}
std::wstring audio_filter::filter_str() const{return u16(impl_->filtergraph_);}
int audio_filter::get_num_output_pads() const { return static_cast<int>(impl_->audio_graph_outputs_.size()); }
const AVFilterLink& audio_filter::get_output_pad_info(int output_pad_id) const { return impl_->get_output_pad_info(output_pad_id); }
std::vector<spl::shared_ptr<AVFrame>> audio_filter::poll_all(int output_pad_id)
{
	std::vector<spl::shared_ptr<AVFrame>> frames;
	for(auto frame = poll(output_pad_id); frame; frame = poll(output_pad_id))
		frames.push_back(spl::make_shared_ptr(frame));
	return frames;
}

ffmpeg::audio_input_pad create_input_pad(const core::video_format_desc& in_format, int num_channels)
{
	return ffmpeg::audio_input_pad(
			boost::rational<int>(1, in_format.audio_sample_rate),
			in_format.audio_sample_rate,
			AVSampleFormat::AV_SAMPLE_FMT_S32,
			av_get_default_channel_layout(num_channels));
}

}}
