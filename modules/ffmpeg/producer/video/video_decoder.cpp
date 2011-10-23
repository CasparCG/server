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

#include <core/producer/frame/basic_frame.h>
#include <common/memory/memcpy.h>

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

#include <connect.h>
#include <semaphore.h>

#include <tbb/scalable_allocator.h>

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
	
struct video_decoder::implementation : public Concurrency::agent, boost::noncopyable
{	
	int										index_;
	std::shared_ptr<AVCodecContext>			codec_context_;
	
	double									fps_;
	int64_t									nb_frames_;

	size_t									width_;
	size_t									height_;
	bool									is_progressive_;
	
	overwrite_buffer<bool>					is_running_;
	unbounded_buffer<safe_ptr<AVPacket>>	source_;
	ITarget<safe_ptr<AVFrame>>&				target_;
	
public:
	explicit implementation(video_decoder::source_t& source, video_decoder::target_t& target, AVFormatContext& context) 
		: codec_context_(open_codec(context, AVMEDIA_TYPE_VIDEO, index_))
		, fps_(static_cast<double>(codec_context_->time_base.den) / static_cast<double>(codec_context_->time_base.num))
		, nb_frames_(context.streams[index_]->nb_frames)
		, width_(codec_context_->width)
		, height_(codec_context_->height)
		, is_progressive_(true)
		, source_([this](const safe_ptr<AVPacket>& packet)
			{
				return packet->stream_index == index_;
			})
		, target_(target)
	{		
		CASPAR_LOG(debug) << "[video_decoder] " << context.streams[index_]->codec->codec->long_name;
		
		CASPAR_VERIFY(width_ > 0, ffmpeg_error());
		CASPAR_VERIFY(height_ > 0, ffmpeg_error());

		Concurrency::connect(source, source_);

		start();
	}

	~implementation()
	{
		send(is_running_, false);
		agent::wait(this);
	}

	virtual void run()
	{
		try
		{
			send(is_running_, true);
			while(is_running_.value())
			{
				auto packet = receive(source_);
			
				if(packet == loop_packet(index_))
				{
					send(target_, loop_video());
					continue;
				}

				if(packet == eof_packet(index_))
					break;

				std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

				int frame_finished = 0;
				THROW_ON_ERROR2(avcodec_decode_video2(codec_context_.get(), decoded_frame.get(), &frame_finished, packet.get()), "[video_decocer]");

				// 1 packet <=> 1 frame.
				// If a decoder consumes less then the whole packet then something is wrong
				// that might be just harmless padding at the end, or a problem with the
				// AVParser or demuxer which puted more then one frame in a AVPacket.

				if(frame_finished == 0)	
					continue;
				
				if(decoded_frame->repeat_pict > 0)
					CASPAR_LOG(warning) << "[video_decoder]: Field repeat_pict not implemented.";
		
				is_progressive_ = decoded_frame->interlaced_frame == 0;
				
				// Need to dupliace frame data since avcodec_decode_video2 reuses it.
				// C-TODO: Avoid duplication.
				send(target_, dup_frame(make_safe_ptr(decoded_frame)));
				Concurrency::wait(10);
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		send(is_running_, false),
		send(target_, eof_video());

		done();
	}

	safe_ptr<AVFrame> dup_frame(const safe_ptr<AVFrame>& frame)
	{
		std::array<uint8_t*, 4> data;
		parallel_for(0, 4, [&](int n)
		{
			data[n] = frame->data[n];
			frame->data[n] = reinterpret_cast<uint8_t*>(scalable_aligned_malloc(frame->linesize[n]*frame->height, 32));
			memcpy(frame->data[n], data[n], frame->linesize[n]*frame->height);
		});

		return safe_ptr<AVFrame>(frame.get(), [frame, data](AVFrame*)
		{
			for(int n = 0; n < 4; ++n)
			{
				scalable_aligned_free(frame->data[n]);
				frame->data[n] = data[n];
			}
		});
	}
		
	double fps() const
	{
		return fps_;
	}
};

video_decoder::video_decoder(video_decoder::source_t& source, video_decoder::target_t& target, AVFormatContext& context) 
	: impl_(new implementation(source, target, context))
{
}

double video_decoder::fps() const{return impl_->fps();}
int64_t video_decoder::nb_frames() const{return impl_->nb_frames_;}
size_t video_decoder::width() const{return impl_->width_;}
size_t video_decoder::height() const{return impl_->height_;}
bool video_decoder::is_progressive() const{return impl_->is_progressive_;}

}}