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

#include "video_decoder.h"

#include "../util/util.h"

#include "../../ffmpeg_error.h"

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

struct video_decoder::implementation : boost::noncopyable
{
	int										index_				= -1;
	const spl::shared_ptr<AVCodecContext>	codec_context_;

	std::queue<spl::shared_ptr<AVPacket>>	packets_;

	const uint32_t							nb_frames_;

	const int								width_				= codec_context_->width;
	const int								height_				= codec_context_->height;
	bool									is_progressive_;

	int										discard_			= 0;
	bool									release_			= true;

	tbb::atomic<uint32_t>					file_frame_number_;

public:
	explicit implementation(const spl::shared_ptr<AVFormatContext>& context)
		: codec_context_(open_codec(*context, AVMEDIA_TYPE_VIDEO, index_, false))
		, nb_frames_(static_cast<uint32_t>(context->streams[index_]->nb_frames))
	{
		file_frame_number_ = 0;

		codec_context_->refcounted_frames = 1;
	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{
		if(!packet)
			return;

		if(packet->stream_index == index_ || packet->data == nullptr)
			packets_.push(spl::make_shared_ptr(packet));
	}

	std::shared_ptr<AVFrame> poll()
	{
		for(int cnt = 0; ; ++cnt)
		{
			if(packets_.empty())
				return nullptr;

			auto packet = packets_.front();

			if(packet->data == nullptr)
			{
				if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)
				{
					auto video = decode(packet);
					if(video)
						return video;
				}

				packets_.pop();

				if(packet->pos != -1)
				{
					file_frame_number_ = static_cast<uint32_t>(packet->pos);
					avcodec_flush_buffers(codec_context_.get());
					return flush_video();
				}
				else // Really EOF
					return nullptr;
			}

			packets_.pop();

			if(release_ || cnt >= discard_)
			{
				release_ = false;
				return decode(packet);
			}

			++file_frame_number_;
		}
	}

	std::shared_ptr<AVFrame> decode(spl::shared_ptr<AVPacket> pkt)
	{
		auto decoded_frame = create_frame();

		int frame_finished = 0;
		THROW_ON_ERROR2(avcodec_decode_video2(codec_context_.get(), decoded_frame.get(), &frame_finished, pkt.get()), "[video_decoder]");

		// If a decoder consumes less then the whole packet then something is wrong
		// that might be just harmless padding at the end, or a problem with the
		// AVParser or demuxer which puted more then one frame in a AVPacket.

		if(frame_finished == 0)
			return nullptr;

		is_progressive_ = !decoded_frame->interlaced_frame;

		if(decoded_frame->repeat_pict > 0)
			CASPAR_LOG(warning) << "[video_decoder] Field repeat_pict not implemented.";

		++file_frame_number_;

		// This ties the life of the decoded_frame to the packet that it came from. For the
		// current version of ffmpeg (0.8 or c17808c) the RAW_VIDEO codec returns frame data
		// owned by the packet.
		return std::shared_ptr<AVFrame>(decoded_frame.get(), [decoded_frame, pkt](AVFrame*){});
	}

	bool ready() const
	{
		return packets_.size() > 8 + discard_;
	}

	bool empty() const
	{
		return packets_.empty();
	}

	uint32_t nb_frames() const
	{
		return std::max(nb_frames_, static_cast<uint32_t>(file_frame_number_));
	}

	void discard(int value)
	{
		discard_ = value;
	}

	std::wstring print() const
	{
		return L"[video-decoder] " + u16(codec_context_->codec->long_name);
	}
};

video_decoder::video_decoder(const spl::shared_ptr<AVFormatContext>& context) : impl_(new implementation(context)){}
void video_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
std::shared_ptr<AVFrame> video_decoder::poll(){return impl_->poll();}
bool video_decoder::ready() const{return impl_->ready();}
bool video_decoder::empty() const { return impl_->empty(); }
int video_decoder::width() const{return impl_->width_;}
int video_decoder::height() const{return impl_->height_;}
uint32_t video_decoder::nb_frames() const{return impl_->nb_frames();}
uint32_t video_decoder::file_frame_number() const{return static_cast<uint32_t>(impl_->file_frame_number_);}
bool video_decoder::is_progressive() const{return impl_->is_progressive_;}
void video_decoder::discard(int value){impl_->discard(value);}
std::wstring video_decoder::print() const{return impl_->print();}

}}
