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

#include "audio_decoder.h"

#include "../util/util.h"
#include "../input/input.h"
#include "../../ffmpeg_error.h"

#include <core/video_format.h>

#include <common/log.h>

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
	#include <libswresample/swresample.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
uint64_t get_channel_layout(AVCodecContext* dec)
{
	auto layout = (dec->channel_layout && dec->channels == av_get_channel_layout_nb_channels(dec->channel_layout)) ? dec->channel_layout : av_get_default_channel_layout(dec->channels);
	return layout;
}

struct audio_decoder::impl : boost::noncopyable
{	
	monitor::basic_subject										event_subject_;
	input*														input_;
	int															index_;
	const spl::shared_ptr<AVCodecContext>						codec_context_;		
	const core::video_format_desc								format_desc_;

	std::shared_ptr<SwrContext>									swr_;

	std::vector<uint8_t, tbb::cache_aligned_allocator<int8_t>>	buffer_;

	std::shared_ptr<AVPacket>									current_packet_;
	
public:
	explicit impl(input& in, const core::video_format_desc& format_desc) 
		: input_(&in)
		, format_desc_(format_desc)	
		, codec_context_(open_codec(input_->context(), AVMEDIA_TYPE_AUDIO, index_))
		, swr_(swr_alloc_set_opts(nullptr,
										av_get_default_channel_layout(format_desc_.audio_channels), AV_SAMPLE_FMT_S32, format_desc_.audio_sample_rate,
										get_channel_layout(codec_context_.get()), codec_context_->sample_fmt, codec_context_->sample_rate,
										0, nullptr), [](SwrContext* p){swr_free(&p);})
		, buffer_(AVCODEC_MAX_AUDIO_FRAME_SIZE*4)
	{		
		if(!swr_)
			CASPAR_THROW_EXCEPTION(bad_alloc());

		THROW_ON_ERROR2(swr_init(swr_.get()), "[audio_decoder]");
	}
		
	std::shared_ptr<AVFrame> poll()
	{		
		if(!current_packet_ && !input_->try_pop_audio(current_packet_))
			return nullptr;
		
		std::shared_ptr<AVFrame> audio;

		if(!current_packet_)	
		{
			avcodec_flush_buffers(codec_context_.get());	
		}
		else if(!current_packet_->data)
		{
			if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)			
				audio = decode(*current_packet_);
			
			if(!audio)
				current_packet_.reset();
		}
		else
		{
			audio = decode(*current_packet_);
			
			if(current_packet_->size == 0)
				current_packet_.reset();
		}
	
		return audio ? audio : poll();
	}

	std::shared_ptr<AVFrame> decode(AVPacket& pkt)
	{		
		auto frame = create_frame();
		
		int got_frame = 0;
		auto len = THROW_ON_ERROR2(avcodec_decode_audio4(codec_context_.get(), frame.get(), &got_frame, &pkt), "[audio_decoder]");
					
		if(len == 0)
		{
			pkt.size = 0;
			return nullptr;
		}

        pkt.data += len;
        pkt.size -= len;

		if(!got_frame)
			return nullptr;
							
		const uint8_t *in[] = {frame->data[0]};
		uint8_t* out[]		= {buffer_.data()};

		auto channel_samples = swr_convert(swr_.get(), 
											out, static_cast<int>(buffer_.size()) / format_desc_.audio_channels / av_get_bytes_per_sample(AV_SAMPLE_FMT_S32), 
											in, frame->nb_samples);	

		frame->data[0]		= buffer_.data();
		frame->linesize[0]	= channel_samples * format_desc_.audio_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
		frame->nb_samples	= channel_samples;
		frame->format		= AV_SAMPLE_FMT_S32;
							
		event_subject_  << monitor::event("file/audio/sample-rate")	% codec_context_->sample_rate
						<< monitor::event("file/audio/channels")	% codec_context_->channels
						<< monitor::event("file/audio/format")		% u8(av_get_sample_fmt_name(codec_context_->sample_fmt))
						<< monitor::event("file/audio/codec")		% u8(codec_context_->codec->long_name);			

		return frame;
	}
	
	uint32_t nb_frames() const
	{
		return 0;
	}

	std::wstring print() const
	{		
		return L"[audio-decoder] " + u16(codec_context_->codec->long_name);
	}
};

audio_decoder::audio_decoder(input& input, const core::video_format_desc& format_desc) : impl_(new impl(input, format_desc)){}
audio_decoder::audio_decoder(audio_decoder&& other) : impl_(std::move(other.impl_)){}
audio_decoder& audio_decoder::operator=(audio_decoder&& other){impl_ = std::move(other.impl_); return *this;}
std::shared_ptr<AVFrame> audio_decoder::operator()(){return impl_->poll();}
uint32_t audio_decoder::nb_frames() const{return impl_->nb_frames();}
std::wstring audio_decoder::print() const{return impl_->print();}
void audio_decoder::subscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.subscribe(o);}
void audio_decoder::unsubscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.unsubscribe(o);}

}}