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

#include "audio_resampler.h"

#include "../util/util.h"
#include "../../ffmpeg_error.h"

#include <core/video_format.h>
#include <core/mixer/audio/audio_util.h>

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
	
struct audio_decoder::implementation : boost::noncopyable
{	
	int																index_;
	const safe_ptr<AVCodecContext>									codec_context_;		
	const AVStream*													stream_;
	const core::video_format_desc									format_desc_;
	
	std::vector<int32_t,  tbb::cache_aligned_allocator<int32_t>>	buffer_;

	std::queue<safe_ptr<AVPacket>>									packets_;

	const int64_t													nb_frames_;
	tbb::atomic<size_t>												file_frame_number_;
	tbb::atomic<int64_t>											packet_time_;
	int64_t															pts_correction_;

	core::channel_layout											channel_layout_;

	std::shared_ptr<SwrContext>										swr_;

public:
	explicit implementation(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc, const std::wstring& custom_channel_order) 
		: format_desc_(format_desc)	
		, codec_context_(open_codec(*context, AVMEDIA_TYPE_AUDIO, index_))
		, buffer_(480000*2)
		, nb_frames_(0)//context->streams[index_]->nb_frames)
		, channel_layout_(get_audio_channel_layout(*codec_context_, custom_channel_order))
		, swr_(swr_alloc_set_opts(nullptr,
								codec_context_->channel_layout ? codec_context_->channel_layout : av_get_default_channel_layout(codec_context_->channels), AV_SAMPLE_FMT_S32, format_desc_.audio_sample_rate,
								codec_context_->channel_layout ? codec_context_->channel_layout : av_get_default_channel_layout(codec_context_->channels), codec_context_->sample_fmt, codec_context_->sample_rate,
								0, nullptr), [](SwrContext* p){swr_free(&p);})
		, stream_(context->streams[index_])
	{	
		if(!swr_)
			BOOST_THROW_EXCEPTION(bad_alloc());
		
		THROW_ON_ERROR2(swr_init(swr_.get()), "[audio_decoder]");

		file_frame_number_ = 0;
		packet_time_ = 0;
		pts_correction_ = stream_->first_dts;
		if (pts_correction_ == AV_NOPTS_VALUE)
			pts_correction_ = 0;

		codec_context_->refcounted_frames = 1;

		CASPAR_LOG(debug) << print() 
				<< " Selected channel layout " << channel_layout_.name;
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
		auto decoded_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* frame)
		{
			av_frame_free(&frame);
		});
				
		int got_frame = 0;
		auto len = THROW_ON_ERROR2(avcodec_decode_audio4(codec_context_.get(), decoded_frame.get(), &got_frame, &pkt), "[audio_decoder]");
					
		if(len == 0)
		{
			pkt.size = 0;
			return nullptr;
		}

        pkt.data += len;
        pkt.size -= len;
					
		if(!got_frame)
			return nullptr;
				
		const uint8_t **in = const_cast<const uint8_t**>(decoded_frame->extended_data);			
		uint8_t* out[]	   = { reinterpret_cast<uint8_t*>(buffer_.data()) };
		
		const auto channel_samples = swr_convert(swr_.get(), 
												 out, static_cast<int>(buffer_.size()) / codec_context_->channels, 
												 in, decoded_frame->nb_samples);
		
		++file_frame_number_;
		if (pkt.pts == AV_NOPTS_VALUE)
			if (stream_->avg_frame_rate.num > 0)
				packet_time_ = (AV_TIME_BASE * static_cast<int64_t>(file_frame_number_) * stream_->avg_frame_rate.den)/stream_->avg_frame_rate.num - pts_correction_;
			else
				packet_time_ = std::numeric_limits<int64_t>().max();
		else
			packet_time_ = ((pkt.pts + pts_correction_) * AV_TIME_BASE * stream_->time_base.num)/stream_->time_base.den;

		return std::make_shared<core::audio_buffer>(buffer_.begin(), buffer_.begin() + channel_samples * decoded_frame->channels);
	}

	bool ready() const
	{
		return packets_.size() > 10;
	}

	bool empty() const
	{
		return packets_.size() == 0;
	}

	uint32_t nb_frames() const
	{
		return 0;//std::max<int64_t>(nb_frames_, file_frame_number_);
	}

	std::wstring print() const
	{		
		return L"[audio-decoder] " + widen(codec_context_->codec->long_name);
	}
};

audio_decoder::audio_decoder(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc, const std::wstring& custom_channel_order) : impl_(new implementation(context, format_desc, custom_channel_order)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
bool audio_decoder::empty() const{return impl_->empty();}
std::shared_ptr<core::audio_buffer> audio_decoder::poll(){return impl_->poll();}
uint32_t audio_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t audio_decoder::file_frame_number() const{return impl_->file_frame_number_;}
const core::channel_layout& audio_decoder::channel_layout() const { return impl_->channel_layout_; }
std::wstring audio_decoder::print() const{return impl_->print();}
int64_t audio_decoder::packet_time() const {return impl_->packet_time_;}

}}