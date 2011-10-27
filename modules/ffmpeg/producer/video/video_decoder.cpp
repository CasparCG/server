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

#include "../util/util.h"
#include "../filter/filter.h"

#include "../../ffmpeg_error.h"

#include <core/producer/frame/basic_frame.h>
#include <common/memory/memcpy.h>
#include <common/concurrency/governor.h>

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

#include <tbb/scalable_allocator.h>

#undef Yield
using namespace Concurrency;

namespace caspar { namespace ffmpeg {
	
struct video_decoder::implementation : public Concurrency::agent, boost::noncopyable
{	
	int										index_;
	const safe_ptr<AVCodecContext>			codec_context_;
	
	const double										fps_;
	const int64_t										nb_frames_;
	const size_t										width_;
	const size_t										height_;
	bool												is_progressive_;
	
	unbounded_buffer<video_decoder::source_element_t>	source_;
	ITarget<video_decoder::target_element_t>&			target_;

	governor											governor_;
	
public:
	explicit implementation(video_decoder::source_t& source, video_decoder::target_t& target, AVFormatContext& context) 
		: codec_context_(open_codec(context, AVMEDIA_TYPE_VIDEO, index_))
		, fps_(static_cast<double>(codec_context_->time_base.den) / static_cast<double>(codec_context_->time_base.num))
		, nb_frames_(context.streams[index_]->nb_frames)
		, width_(codec_context_->width)
		, height_(codec_context_->height)
		, is_progressive_(true)
		, source_([this](const video_decoder::source_element_t& packet){return packet->stream_index == index_;})
		, target_(target)
		, governor_(1) // IMPORTANT: Must be 1 since avcodec_decode_video2 reuses memory.
	{		
		CASPAR_LOG(debug) << "[video_decoder] " << context.streams[index_]->codec->codec->long_name;
		
		source.link_target(&source_);
		
		start();
	}

	~implementation()
	{
		governor_.cancel();
		agent::wait(this);
	}

	virtual void run()
	{
		try
		{
			while(true)
			{
				auto ticket = governor_.acquire();
				auto packet = receive(source_);
			
				if(packet == flush_packet(index_))
				{					
					if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)
					{
						AVPacket pkt;
						av_init_packet(&pkt);
						pkt.data = nullptr;
						pkt.size = 0;

						for(auto decoded_frame = decode(pkt); decoded_frame; decoded_frame = decode(pkt))
						{						
							send(target_, safe_ptr<AVFrame>(decoded_frame.get(), [decoded_frame, ticket](AVFrame*){}));
							Context::Yield();
						}
					}

					avcodec_flush_buffers(codec_context_.get());
					send(target_, flush_video());
					continue;
				}

				if(packet == eof_packet(index_))
					break;

				auto decoded_frame = decode(*packet);
				if(!decoded_frame)
					continue;
		
				is_progressive_ = decoded_frame->interlaced_frame == 0;
				
				// C-TODO: Avoid duplication.
				// Need to dupliace frame data since avcodec_decode_video2 reuses it.
				send(target_, safe_ptr<AVFrame>(decoded_frame.get(), [decoded_frame, ticket](AVFrame*){}));				
				Context::Yield();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		send(target_, eof_video());

		done();
	}
	
	std::shared_ptr<AVFrame> decode(AVPacket& packet)
	{
		std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		THROW_ON_ERROR2(avcodec_decode_video2(codec_context_.get(), decoded_frame.get(), &frame_finished, &packet), "[video_decocer]");

		// 1 packet <=> 1 frame.
		// If a decoder consumes less then the whole packet then something is wrong
		// that might be just harmless padding at the end, or a problem with the
		// AVParser or demuxer which puted more then one frame in a AVPacket.

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