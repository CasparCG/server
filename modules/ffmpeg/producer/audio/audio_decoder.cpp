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

#include "../../stdafx.h"

#include "audio_decoder.h"

#include "../util/util.h"
#include "../../util/error.h"

#include <core/video_format.h>
#include <core/mixer/audio/audio_util.h>

#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <tbb/cache_aligned_allocator.h>

#include <queue>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
	#include <libavutil/opt.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
struct audio_decoder::implementation : boost::noncopyable
{	
	int								index_;
	const safe_ptr<AVCodecContext>	codec_context_;		
	const core::video_format_desc	format_desc_;

    AVFilterContext*				audio_graph_in_;  
    AVFilterContext*				audio_graph_out_; 
    std::shared_ptr<AVFilterGraph>	audio_graph_;    

	std::queue<safe_ptr<AVPacket>>	packets_;

	const int64_t					nb_frames_;
	tbb::atomic<size_t>				file_frame_number_;
public:
	explicit implementation(
		const safe_ptr<AVFormatContext>& context, 
		const core::video_format_desc& format_desc, 
		const std::string& filtergraph) 
		: format_desc_(format_desc)	
		, codec_context_(
			open_codec(
				*context, 
				AVMEDIA_TYPE_AUDIO, 
				index_))
		, nb_frames_(0)//context->streams[index_]->nb_frames)
	{
		file_frame_number_ = 0;
		
		codec_context_->refcounted_frames = 1;

		configure_audio_filters(filtergraph);
	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{			
		if(!packet)
			return;

		if(packet->stream_index == index_ || packet->data == nullptr)
			packets_.push(make_safe_ptr(packet));
	}	
	
	std::shared_ptr<core::audio_buffer> poll()
	{
		if(packets_.empty())
			return nullptr;
				
		auto packet = packets_.front();

		if(packet->data == nullptr)
		{
			packets_.pop();
			file_frame_number_ = static_cast<size_t>(packet->pos);
			avcodec_flush_buffers(codec_context_.get());
			return flush_audio();
		}

		auto audio = decode(*packet);

		if(packet->size == 0)					
			packets_.pop();

		return audio;
	}

	std::shared_ptr<core::audio_buffer> decode(AVPacket& pkt)
	{			
		auto frame = std::shared_ptr<AVFrame>(
			av_frame_alloc(), 
			[](AVFrame* frame)
			{
				av_frame_free(&frame);
			});
		
		int got_picture = 0;

		auto len = avcodec_decode_audio4(
				codec_context_.get(), 
				frame.get(),
				&got_picture, 
				&pkt);
		
		// There might be several frames in one packet.
		pkt.size -= len;
		pkt.data += len;
			
		if(!got_picture)
			return std::make_shared<core::audio_buffer>();
				
		FF(av_buffersrc_add_frame(
			audio_graph_in_, 
			frame.get()));
				
		auto result = std::make_shared<core::audio_buffer>();

		while(true)
		{
			std::shared_ptr<AVFrame> filt_frame(
				av_frame_alloc(), 
				[](AVFrame* p)
				{
					av_frame_free(&p);
				});

			if(av_buffersink_get_frame(
				audio_graph_out_, 
				filt_frame.get()) < 0)
			{
				break;
			}

			CASPAR_ASSERT(av_get_bytes_per_sample(static_cast<AVSampleFormat>(filt_frame->format)) == sizeof((*result)[0]));

			result->insert(
				result->end(),
				reinterpret_cast<std::int32_t*>(filt_frame->data[0]),
				reinterpret_cast<std::int32_t*>(filt_frame->data[0]) + filt_frame->nb_samples * filt_frame->channels);
		}
				
		++file_frame_number_;

		return result;
	}

	bool ready() const
	{
		return packets_.size() > 10;
	}

	uint32_t nb_frames() const
	{
		return 0;//std::max<int64_t>(nb_frames_, file_frame_number_);
	}

	std::wstring print() const
	{		
		return L"[audio-decoder] " + widen(codec_context_->codec->long_name);
	}
		
	void configure_audio_filters(const std::string& filtergraph)
	{
		audio_graph_.reset(
			avfilter_graph_alloc(), 
			[](AVFilterGraph* p)
			{
				avfilter_graph_free(&p);
			});

		audio_graph_->nb_threads  = boost::thread::hardware_concurrency()/2;
		audio_graph_->thread_type = AVFILTER_THREAD_SLICE;
				
		auto asrc_options = (boost::format("sample_rate=%1%:sample_fmt=%2%:channels=%3%:time_base=%4%/%5%")
			% codec_context_->sample_rate
			% av_get_sample_fmt_name(codec_context_->sample_fmt)
			% codec_context_->channels
			% codec_context_->time_base.num % codec_context_->time_base.den).str();

		if(codec_context_->channel_layout)
		{
			asrc_options += (boost::format(":channel_layout=%6%") 
				% boost::io::group(
					std::hex, 
					std::showbase, 
					codec_context_->channel_layout)).str();
		}

		AVFilterContext* filt_asrc = nullptr;
		FF(avfilter_graph_create_filter(
			&filt_asrc,
			avfilter_get_by_name("abuffer"), 
			"ffmpeg_consumer_abuffer",
			asrc_options.c_str(), 
			nullptr, 
			audio_graph_.get()));
				
		AVFilterContext* filt_asink = nullptr;
		FF(avfilter_graph_create_filter(
			&filt_asink,
			avfilter_get_by_name("abuffersink"), 
			"ffmpeg_consumer_abuffersink",
			nullptr, 
			nullptr, 
			audio_graph_.get()));

		const AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S32, AV_SAMPLE_FMT_NONE }; 
		const int sample_rates[2]		   = { core::video_format_desc::audio_sample_rate, -1 };
		const int64_t channel_layouts[2]   = { av_get_default_channel_layout(core::video_format_desc::audio_nb_channels), -1 };
		const int channels[2]			   = { core::video_format_desc::audio_nb_channels, -1 };

#pragma warning (push)
#pragma warning (disable : 4245)

		FF(av_opt_set_int(
			filt_asink,	   
			"all_channel_counts", 
			0,	
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"sample_fmts",		 
			sample_fmts,					
			-1, 
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"channel_layouts",	 
			channel_layouts,				
			-1, 
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"channel_counts",	 
			channels,						
			-1, 
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"sample_rates",		 
			sample_rates,					
			-1,
			AV_OPT_SEARCH_CHILDREN));

#pragma warning (pop)
			
		configure_filtergraph(
			*audio_graph_, 
			filtergraph, 
			*filt_asrc, 
			*filt_asink);

		audio_graph_in_  = filt_asrc;
		audio_graph_out_ = filt_asink;

		CASPAR_LOG(info) 
			<< widen(std::string("\n") 
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
					&inputs, 
					&outputs,
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
			avfilter_inout_free(&outputs);
			avfilter_inout_free(&inputs);
			throw;
		}
	}
};

audio_decoder::audio_decoder(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc, const std::wstring& filtergraph) : impl_(new implementation(context, format_desc, narrow(filtergraph))){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::shared_ptr<core::audio_buffer> audio_decoder::poll(){return impl_->poll();}
uint32_t audio_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t audio_decoder::file_frame_number() const{return impl_->file_frame_number_;}
std::wstring audio_decoder::print() const{return impl_->print();}

}}