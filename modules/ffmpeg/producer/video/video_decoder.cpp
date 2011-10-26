/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../../stdafx.h"

#include "video_decoder.h"

#include "../util.h"
#include "../filter/filter.h"

#include "../../ffmpeg_error.h"

#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/frame_factory.h>

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

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
	
struct video_decoder::implementation : boost::noncopyable
{
	video_decoder::source_t&				source_;
	const safe_ptr<core::frame_factory>		frame_factory_;
	int										index_;
	const safe_ptr<AVCodecContext>			codec_context_;

	std::queue<safe_ptr<AVPacket>>			packets_;
	
	const double							fps_;
	const int64_t							nb_frames_;
	const size_t							width_;
	const size_t							height_;
	
public:
	explicit implementation(video_decoder::source_t& source, const safe_ptr<AVFormatContext>& context, const safe_ptr<core::frame_factory>& frame_factory) 
		: source_(source)
		, frame_factory_(frame_factory)
		, codec_context_(open_codec(*context, AVMEDIA_TYPE_VIDEO, index_))
		, fps_(static_cast<double>(codec_context_->time_base.den) / static_cast<double>(codec_context_->time_base.num))
		, nb_frames_(context->streams[index_]->nb_frames)
		, width_(codec_context_->width)
		, height_(codec_context_->height)
	{			
	}
	
	std::shared_ptr<AVFrame> poll()
	{		
		auto packet = create_packet();
		
		if(packets_.empty())
		{
			if(!try_receive(source_, packet) || packet->stream_index != index_)
				return nullptr;
			else
				packets_.push(packet);
		}
		
		packet = packets_.front();

		std::shared_ptr<AVFrame> video;

		if(packet == loop_packet())
		{
			if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)
			{
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = nullptr;
				pkt.size = 0;

				video = decode(pkt);
				if(video)
					packets_.push(packet);
			}

			if(!video)
			{
				avcodec_flush_buffers(codec_context_.get());
				video = loop_video();
			}
		}			
		else
			video = decode(*packet);

		if(packet->size == 0)
			packets_.pop();
						
		return video;
	}
	
	std::shared_ptr<AVFrame> decode(AVPacket& pkt)
	{
		std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		THROW_ON_ERROR2(avcodec_decode_video2(codec_context_.get(), decoded_frame.get(), &frame_finished, &pkt), "[video_decocer]");
		
		// If a decoder consumes less then the whole packet then something is wrong
		// that might be just harmless padding at the end, or a problem with the
		// AVParser or demuxer which puted more then one frame in a AVPacket.
		pkt.data = nullptr;
		pkt.size = 0;

		if(frame_finished == 0)	
			return nullptr;

		if(decoded_frame->repeat_pict > 0)
			CASPAR_LOG(warning) << "[video_decoder]: Field repeat_pict not implemented.";
		
		return decoded_frame;
	}
	
	double fps() const
	{
		return fps_;
	}
};

video_decoder::video_decoder(video_decoder::source_t& source, const safe_ptr<AVFormatContext>& context, const safe_ptr<core::frame_factory>& frame_factory) : impl_(new implementation(source, context, frame_factory)){}
std::shared_ptr<AVFrame> video_decoder::poll(){return impl_->poll();}
double video_decoder::fps() const{return impl_->fps();}
int64_t video_decoder::nb_frames() const{return impl_->nb_frames_;}
size_t video_decoder::width() const{return impl_->width_;}
size_t video_decoder::height() const{return impl_->height_;}

}}