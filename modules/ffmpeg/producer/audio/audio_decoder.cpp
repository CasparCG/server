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
	int															index_;
	const safe_ptr<AVCodecContext>								codec_context_;		
	const core::video_format_desc								format_desc_;
	
	std::shared_ptr<SwrContext>									swr_;
	
	std::queue<safe_ptr<AVPacket>>								packets_;

	const int64_t												nb_frames_;
	tbb::atomic<size_t>											file_frame_number_;
public:
	explicit implementation(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)	
		, codec_context_(open_codec(*context, AVMEDIA_TYPE_AUDIO, index_))
		, nb_frames_(0)//context->streams[index_]->nb_frames)
	{
		file_frame_number_ = 0;
		
		codec_context_->refcounted_frames = 1;

		swr_.reset(swr_alloc_set_opts(nullptr,
					av_get_default_channel_layout(format_desc_.audio_nb_channels), AV_SAMPLE_FMT_S32, 48000,
					codec_context_->channel_layout != 0 ? codec_context_->channel_layout : av_get_default_channel_layout(codec_context_->channels), codec_context_->sample_fmt, codec_context_->sample_rate, 
					0, nullptr),
					[](SwrContext* p){swr_free(&p);});
			
		swr_init(swr_.get());
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
		auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* frame)
		{
			av_frame_free(&frame);
		});
		
		int got_picture = 0;
		auto len = avcodec_decode_audio4(codec_context_.get(), frame.get(), &got_picture, &pkt);
		
		// There might be several frames in one packet.
		pkt.size -= len;
		pkt.data += len;
			
		if(!got_picture)
			return std::make_shared<core::audio_buffer>();
		
		auto out_buffer = core::audio_buffer(frame->nb_samples * core::video_format_desc::audio_nb_channels * 2);

		uint8_t* out[] = {reinterpret_cast<uint8_t*>(out_buffer.data())};
		const auto in = const_cast<const uint8_t**>(frame->extended_data);	
				
		auto out_samples2 = swr_convert(swr_.get(), 
								out, 
								static_cast<int>(out_buffer.size()) / codec_context_->channels, 
								in, 
								frame->nb_samples);	
			
		out_buffer.resize(out_samples2 * core::video_format_desc::audio_nb_channels);

		++file_frame_number_;

		return std::make_shared<core::audio_buffer>(std::move(out_buffer));
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
};

audio_decoder::audio_decoder(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new implementation(context, format_desc)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::shared_ptr<core::audio_buffer> audio_decoder::poll(){return impl_->poll();}
uint32_t audio_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t audio_decoder::file_frame_number() const{return impl_->file_frame_number_;}
std::wstring audio_decoder::print() const{return impl_->print();}

}}