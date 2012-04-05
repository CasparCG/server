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

#include "video_decoder.h"

#include "../util/util.h"
#include "../input/input.h"

#include "../../ffmpeg_error.h"

#include <common/log.h>
#include <core/frame/frame_transform.h>
#include <core/frame/frame_factory.h>

#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/filesystem.hpp>

#include <queue>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
struct video_decoder::impl : boost::noncopyable
{
	monitor::basic_subject					event_subject_;
	input*									input_;
	int										index_;
	const std::shared_ptr<AVCodecContext>	codec_context_;

	std::queue<spl::shared_ptr<AVPacket>>	packets_;
	
	const AVStream*							stream_;
	const uint32_t							nb_frames_;
	const int								width_;
	const int								height_;

	bool									is_progressive_;
	uint32_t								file_frame_number_;
	double									fps_;

public:
	explicit impl() 
		: input_(nullptr)
		, nb_frames_(0)
		, width_(0)
		, height_(0)
		, file_frame_number_(0)
		, fps_(0.0)
	{
	}

	explicit impl(input& in) 
		: input_(&in)
		, codec_context_(open_codec(input_->context(), AVMEDIA_TYPE_VIDEO, index_))
		, stream_(input_->context().streams[index_])
		, nb_frames_(static_cast<uint32_t>(stream_->nb_frames))
		, width_(codec_context_->width)
		, height_(codec_context_->height)
		, file_frame_number_(0)
		, fps_(read_fps(input_->context(), 0.0))
	{
	}
	
	std::vector<std::shared_ptr<AVFrame>> poll()
	{			
		std::vector<std::shared_ptr<AVFrame>> result;

		if(!codec_context_)
		{
			result.push_back(empty_video());
			return result;
		}

		std::shared_ptr<AVPacket> packet;
		if(!input_->try_pop_video(packet))
			return result;

		if(packet == flush_packet())
		{
			avcodec_flush_buffers(codec_context_.get());
			return result;
		}
		
		if(packet == null_packet())
		{
			if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)
			{
				AVPacket pkt;                
				av_init_packet(&pkt);
				pkt.data = nullptr;
				pkt.size = 0;
				
				std::shared_ptr<AVFrame> frame;
				while(decode(pkt, frame))
					result.push_back(frame);	
			}
			return result;
		}

		while(packet->size > 0)
		{
			std::shared_ptr<AVFrame> frame;
			if(decode(*packet, frame))
				result.push_back(frame);				
		}
		
		return result;
	}

	bool decode(AVPacket& pkt, std::shared_ptr<AVFrame>& result)
	{
		result = std::shared_ptr<AVFrame>(avcodec_alloc_frame(), av_free);

		int got_frame = 0;
		auto len = THROW_ON_ERROR2(avcodec_decode_video2(codec_context_.get(), result.get(), &got_frame, &pkt), "[video_decocer]");
				
		if(len == 0)
		{
			pkt.size = 0;
			return false;
		}

        pkt.data += len;
        pkt.size -= len;

		if(got_frame == 0)	
			return false;
		
		auto stream_time_base	 = stream_->time_base;
		auto packet_frame_number = static_cast<uint32_t>((static_cast<double>(pkt.pts * stream_time_base.num)/stream_time_base.den)*fps_);

		file_frame_number_ = packet_frame_number;

		is_progressive_ = !result->interlaced_frame;
		
		if(result->repeat_pict > 0)
			CASPAR_LOG(warning) << "[video_decoder] repeat_pict not implemented.";
				
		event_subject_  << monitor::event("file/video/width")	% width_
						<< monitor::event("file/video/height")	% height_
						<< monitor::event("file/video/field")	% u8(!result->interlaced_frame ? "progressive" : (result->top_field_first ? "upper" : "lower"))
						<< monitor::event("file/video/codec")	% u8(codec_context_->codec->long_name);
		
		return true;
	}
	
	uint32_t nb_frames() const
	{
		return std::max(nb_frames_, file_frame_number_);
	}

	std::wstring print() const
	{		
		return L"[video-decoder] " + u16(codec_context_->codec->long_name);
	}
};

video_decoder::video_decoder() : impl_(new impl()){}
video_decoder::video_decoder(input& in) : impl_(new impl(in)){}
video_decoder::video_decoder(video_decoder&& other) : impl_(std::move(other.impl_)){}
video_decoder& video_decoder::operator=(video_decoder&& other){impl_ = std::move(other.impl_); return *this;}
std::vector<std::shared_ptr<AVFrame>> video_decoder::operator()(){return impl_->poll();}
int video_decoder::width() const{return impl_->width_;}
int video_decoder::height() const{return impl_->height_;}
uint32_t video_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t video_decoder::file_frame_number() const{return impl_->file_frame_number_;}
bool video_decoder::is_progressive() const{return impl_->is_progressive_;}
std::wstring video_decoder::print() const{return impl_->print();}
void video_decoder::subscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.subscribe(o);}
void video_decoder::unsubscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.unsubscribe(o);}

}}