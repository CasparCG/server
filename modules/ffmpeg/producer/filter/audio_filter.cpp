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

#include <common/assert.h>
#include <common/except.h>

#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/rational.hpp>

#include <cstdio>
#include <sstream>
#include <string>

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
	
struct audio_filter::implementation
{
	std::string						filtergraph_;

	std::shared_ptr<AVFilterGraph>	audio_graph_;	
    AVFilterContext*				audio_graph_in_;  
    AVFilterContext*				audio_graph_out_; 

	implementation(
			boost::rational<int> in_time_base,
			int in_sample_rate,
			AVSampleFormat in_sample_fmt,
			std::int64_t in_audio_channel_layout,
			std::vector<int> out_sample_rates,
			std::vector<AVSampleFormat> out_sample_fmts,
			std::vector<std::int64_t> out_audio_channel_layouts,
			const std::string& filtergraph)
		: filtergraph_(boost::to_lower_copy(filtergraph))
	{
		if (out_sample_rates.empty())
			out_sample_rates.push_back(48000);

		out_sample_rates.push_back(-1);

		if (out_sample_fmts.empty())
			out_sample_fmts.push_back(AV_SAMPLE_FMT_S32);

		out_sample_fmts.push_back(AV_SAMPLE_FMT_NONE);

		if (out_audio_channel_layouts.empty())
			out_audio_channel_layouts.push_back(AV_CH_LAYOUT_NATIVE);

		out_audio_channel_layouts.push_back(-1);

		audio_graph_.reset(
			avfilter_graph_alloc(), 
			[](AVFilterGraph* p)
			{
				avfilter_graph_free(&p);
			});

		const auto asrc_options = (boost::format("time_base=%1%/%2%:sample_rate=%3%:sample_fmt=%4%:channel_layout=0x%|5$x|")
			% in_time_base.numerator() % in_time_base.denominator()
			% in_sample_rate
			% av_get_sample_fmt_name(in_sample_fmt)
			% in_audio_channel_layout).str();
					
		AVFilterContext* filt_asrc = nullptr;			
		FF(avfilter_graph_create_filter(
			&filt_asrc,
			avfilter_get_by_name("abuffer"), 
			"filter_buffer",
			asrc_options.c_str(),
			nullptr, 
			audio_graph_.get()));
				
		AVFilterContext* filt_asink = nullptr;
		FF(avfilter_graph_create_filter(
			&filt_asink,
			avfilter_get_by_name("abuffersink"), 
			"filter_buffersink",
			nullptr, 
			nullptr, 
			audio_graph_.get()));
		
#pragma warning (push)
#pragma warning (disable : 4245)

		FF(av_opt_set_int_list(
			filt_asink,
			"sample_fmts",
			out_sample_fmts.data(),
			-1,
			AV_OPT_SEARCH_CHILDREN));
		FF(av_opt_set_int_list(
			filt_asink,
			"channel_layouts",
			out_audio_channel_layouts.data(),
			-1,
			AV_OPT_SEARCH_CHILDREN));
		FF(av_opt_set_int_list(
			filt_asink,
			"sample_rates",
			out_sample_rates.data(),
			-1,
			AV_OPT_SEARCH_CHILDREN));

#pragma warning (pop)
			
		configure_filtergraph(
				*audio_graph_,
				filtergraph_,
				*filt_asrc,
				*filt_asink);

		audio_graph_in_  = filt_asrc;
		audio_graph_out_ = filt_asink;
		
		CASPAR_LOG(info)
			<< 	u16(std::string("\n") 
				+ avfilter_graph_dump(
						audio_graph_.get(), 
						nullptr));
	}
	
	void configure_filtergraph(
			AVFilterGraph& graph,
			const std::string& filtergraph,
			AVFilterContext& source_ctx,
			AVFilterContext& sink_ctx)
	{
		AVFilterInOut* outputs = nullptr;
		AVFilterInOut* inputs = nullptr;

		try
		{
			if(!filtergraph.empty()) 
			{
				outputs = avfilter_inout_alloc();
				inputs  = avfilter_inout_alloc();

				CASPAR_VERIFY(outputs && inputs);

				outputs->name       = av_strdup("in");
				outputs->filter_ctx = &source_ctx;
				outputs->pad_idx    = 0;
				outputs->next       = nullptr;

				inputs->name        = av_strdup("out");
				inputs->filter_ctx  = &sink_ctx;
				inputs->pad_idx     = 0;
				inputs->next        = nullptr;

				FF(avfilter_graph_parse(
					&graph, 
					filtergraph.c_str(), 
					inputs,
					outputs,
					nullptr));
			} 
			else 
			{
				FF(avfilter_link(
					&source_ctx, 
					0, 
					&sink_ctx, 
					0));
			}

			FF(avfilter_graph_config(
				&graph, 
				nullptr));
		}
		catch(...)
		{
			//avfilter_inout_free(&outputs);
			//avfilter_inout_free(&inputs);
			throw;
		}
	}

	void push(const std::shared_ptr<AVFrame>& src_av_frame)
	{		
		FF(av_buffersrc_add_frame(
			audio_graph_in_, 
			src_av_frame.get()));
	}

	std::shared_ptr<AVFrame> poll()
	{
		std::shared_ptr<AVFrame> filt_frame(
			av_frame_alloc(), 
			[](AVFrame* p)
			{
				av_frame_free(&p);
			});
		
		const auto ret = av_buffersink_get_frame(
			audio_graph_out_, 
			filt_frame.get());
				
		if(ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
			return nullptr;
					
		FF_RET(ret, "poll");

		return filt_frame;
	}
};

audio_filter::audio_filter(
		boost::rational<int> in_time_base,
		int in_sample_rate,
		AVSampleFormat in_sample_fmt,
		std::int64_t in_audio_channel_layout,
		std::vector<int> out_sample_rates,
		std::vector<AVSampleFormat> out_sample_fmts,
		std::vector<std::int64_t> out_audio_channel_layouts,
		const std::string& filtergraph)
	: impl_(new implementation(
		in_time_base,
		in_sample_rate,
		in_sample_fmt,
		in_audio_channel_layout,
		std::move(out_sample_rates),
		std::move(out_sample_fmts),
		std::move(out_audio_channel_layouts),
		filtergraph)){}
audio_filter::audio_filter(audio_filter&& other) : impl_(std::move(other.impl_)){}
audio_filter& audio_filter::operator=(audio_filter&& other){impl_ = std::move(other.impl_); return *this;}
void audio_filter::push(const std::shared_ptr<AVFrame>& frame){impl_->push(frame);}
std::shared_ptr<AVFrame> audio_filter::poll(){return impl_->poll();}
std::wstring audio_filter::filter_str() const{return u16(impl_->filtergraph_);}
std::vector<spl::shared_ptr<AVFrame>> audio_filter::poll_all()
{	
	std::vector<spl::shared_ptr<AVFrame>> frames;
	for(auto frame = poll(); frame; frame = poll())
		frames.push_back(spl::make_shared_ptr(frame));
	return frames;
}

}}
