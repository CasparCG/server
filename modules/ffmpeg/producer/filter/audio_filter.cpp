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

std::string create_filter_list(const std::vector<std::string>& items)
{
	return boost::join(items, "|");
}

std::string channel_layout_to_string(int64_t channel_layout)
{
	return (boost::format("0x%|1$x|") % channel_layout).str();
}

std::string create_sinkfilter_str(const audio_output_pad& output_pad, std::string name)
{
	const auto asink_options = (boost::format("[%4%] abuffersink")//=sample_fmts=%1%:channel_layouts=%2%:sample_rates=%3%")
		% create_filter_list(cpplinq::from(output_pad.sample_fmts)
				.select(&av_get_sample_fmt_name)
				.select([](const char* str) { return std::string(str); })
				.to_vector())
		% create_filter_list(cpplinq::from(output_pad.sample_fmts)
				.select(&channel_layout_to_string)
				.to_vector())
		% create_filter_list(cpplinq::from(output_pad.sample_rates)
				.select([](int samplerate) { return boost::lexical_cast<std::string>(samplerate); })
				.to_vector())
		% name).str();

	return asink_options;
}

struct audio_filter::implementation
{
	std::string						filtergraph_;

	std::shared_ptr<AVFilterGraph>	audio_graph_;
	std::vector<AVFilterContext*>	audio_graph_inputs_;
	std::vector<AVFilterContext*>	audio_graph_outputs_;

	implementation(
		std::vector<audio_input_pad> input_pads,
		std::vector<audio_output_pad> output_pads,
		const std::string& filtergraph)
		: filtergraph_(boost::to_lower_copy(filtergraph))
	{
		if (input_pads.empty())
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
			for (auto& input_pad : input_pads)
				complete_filter_graph.push_back(create_sourcefilter_str(input_pad, "a:" + boost::lexical_cast<std::string>(i++)));
		}

		if (filtergraph_.empty())
			complete_filter_graph.push_back("[a:0] anull [aout:0]");
		else
			complete_filter_graph.push_back(filtergraph_);

		{
			int i = 0;
			for (auto& output_pad : output_pads)
				complete_filter_graph.push_back(create_sinkfilter_str(output_pad, "aout:" + boost::lexical_cast<std::string>(i++)));
		}

		configure_filtergraph(
				*audio_graph_,
				boost::join(complete_filter_graph, ";"),
				audio_graph_inputs_,
				audio_graph_outputs_);
		
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
			std::vector<AVFilterContext*>& sink_contexts)
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

		FF(avfilter_graph_config(
			&graph, 
			nullptr));
	}

	void push(int input_pad_id, const std::shared_ptr<AVFrame>& src_av_frame)
	{		
		FF(av_buffersrc_add_frame(
			audio_graph_inputs_.at(input_pad_id),
			src_av_frame.get()));
	}

	std::shared_ptr<AVFrame> poll(int output_pad_id)
	{
		std::shared_ptr<AVFrame> filt_frame(
			av_frame_alloc(), 
			[](AVFrame* p)
			{
				av_frame_free(&p);
			});
		
		const auto ret = av_buffersink_get_frame(
			audio_graph_outputs_.at(output_pad_id),
			filt_frame.get());
				
		if(ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
			return nullptr;
					
		FF_RET(ret, "poll");

		return filt_frame;
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
void audio_filter::push(int input_pad_id, const std::shared_ptr<AVFrame>& frame){impl_->push(input_pad_id, frame);}
std::shared_ptr<AVFrame> audio_filter::poll(int output_pad_id){return impl_->poll(output_pad_id);}
std::wstring audio_filter::filter_str() const{return u16(impl_->filtergraph_);}
std::vector<spl::shared_ptr<AVFrame>> audio_filter::poll_all(int output_pad_id)
{	
	std::vector<spl::shared_ptr<AVFrame>> frames;
	for(auto frame = poll(output_pad_id); frame; frame = poll(output_pad_id))
		frames.push_back(spl::make_shared_ptr(frame));
	return frames;
}

}}
