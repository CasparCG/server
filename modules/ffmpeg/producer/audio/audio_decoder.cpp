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

#include "audio_resampler.h"

#include "../util/util.h"
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
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
struct audio_decoder::impl : boost::noncopyable
{	
	monitor::basic_subject										event_subject_;
	int															index_;
	const std::shared_ptr<AVCodecContext>						codec_context_;		
	const core::video_format_desc								format_desc_;

	boost::optional<audio_resampler>							resampler_;

	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer1_;

	std::queue<spl::shared_ptr<AVPacket>>						packets_;

	const int64_t												nb_frames_;
	uint32_t													file_frame_number_;
public:
	explicit impl() 
		: nb_frames_(0)//context->streams[index_]->nb_frames)
		, file_frame_number_(0)
	{		
	}

	explicit impl(const std::shared_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)	
		, codec_context_(open_codec(*context, AVMEDIA_TYPE_AUDIO, index_))
		, resampler_(audio_resampler(format_desc.audio_channels,	codec_context_->channels,
					 format_desc.audio_sample_rate, codec_context_->sample_rate,
					 AV_SAMPLE_FMT_S32,				codec_context_->sample_fmt))
		, buffer1_(AVCODEC_MAX_AUDIO_FRAME_SIZE*2)
		, nb_frames_(0)//context->streams[index_]->nb_frames)
	{		
		file_frame_number_ = 0;   
	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{			
		if(!packet)
			return;

		if(packet->stream_index == index_ || packet->data == nullptr)
			packets_.push(spl::make_shared_ptr(packet));
	}	
	
	std::shared_ptr<core::audio_buffer> poll()
	{
		if(packets_.empty())
			return nullptr;
				
		auto packet = packets_.front();
		
		if(!codec_context_)		
		{
			packets_.pop();
			return packet->data == nullptr ? flush_audio() : empty_audio();
		}
		else
		{
			if(packet->data == nullptr)
			{
				packets_.pop();
				file_frame_number_ = static_cast<uint32_t>(packet->pts);
				avcodec_flush_buffers(codec_context_.get());
				return flush_audio();
			}

			auto audio = decode(*packet);

			if(packet->size == 0)					
				packets_.pop();

			return audio;
		}
	}

	std::shared_ptr<core::audio_buffer> decode(AVPacket& pkt)
	{		
		buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);
		int written_bytes = static_cast<int>(buffer1_.size()) - FF_INPUT_BUFFER_PADDING_SIZE;
		
		int ret = THROW_ON_ERROR2(avcodec_decode_audio3(codec_context_.get(), reinterpret_cast<int16_t*>(buffer1_.data()), &written_bytes, &pkt), "[audio_decoder]");

		// There might be several frames in one packet.
		pkt.size -= ret;
		pkt.data += ret;
			
		buffer1_.resize(written_bytes);

		buffer1_ = resampler_->resample(std::move(buffer1_));
		
		const auto n_samples = buffer1_.size() / av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
		const auto samples = reinterpret_cast<int32_t*>(buffer1_.data());
		
		event_subject_  << monitor::event("file/audio/sample-rate")	% codec_context_->sample_rate
						<< monitor::event("file/audio/channels")	% codec_context_->channels
						<< monitor::event("file/audio/format")		% u8(av_get_sample_fmt_name(codec_context_->sample_fmt))
						<< monitor::event("file/audio/codec")		% u8(codec_context_->codec->long_name);
		
		file_frame_number_ = static_cast<uint32_t>(pkt.pts);
		
		return std::make_shared<core::audio_buffer>(samples, samples + n_samples);
	}

	bool ready() const
	{
		return !codec_context_ || !packets_.empty();
	}
	
	void clear()
	{
		while(!packets_.empty())
			packets_.pop();
	}

	uint32_t nb_frames() const
	{
		return 0;//std::max<int64_t>(nb_frames_, file_frame_number_);
	}

	std::wstring print() const
	{		
		return L"[audio-decoder] " + u16(codec_context_->codec->long_name);
	}
};

audio_decoder::audio_decoder() : impl_(new impl()){}
audio_decoder::audio_decoder(const std::shared_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new impl(context, format_desc)){}
audio_decoder::audio_decoder(audio_decoder&& other) : impl_(std::move(other.impl_)){}
audio_decoder& audio_decoder::operator=(audio_decoder&& other){impl_ = std::move(other.impl_); return *this;}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::shared_ptr<core::audio_buffer> audio_decoder::poll(){return impl_->poll();}
uint32_t audio_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t audio_decoder::file_frame_number() const{return impl_->file_frame_number_;}
std::wstring audio_decoder::print() const{return impl_->print();}
void audio_decoder::clear(){impl_->clear();}
void audio_decoder::subscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.subscribe(o);}
void audio_decoder::unsubscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.unsubscribe(o);}

}}