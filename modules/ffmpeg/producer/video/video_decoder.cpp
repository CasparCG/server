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
#include "../../tbb_avcodec.h"

#include <core/producer/frame/basic_frame.h>

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
	
struct video_decoder::implementation : public Concurrency::agent, boost::noncopyable
{
	video_decoder::source_t&				source_;
	video_decoder::forward_t&				forward_;
	video_decoder::target_t&				target_;

	std::shared_ptr<AVCodecContext>			codec_context_;
	int										index_;

	filter									filter_;

	double									fps_;
	int64_t									nb_frames_;

	size_t									width_;
	size_t									height_;
	bool									is_progressive_;

public:
	explicit implementation(video_decoder::source_t& source,
							video_decoder::forward_t& forward,
							video_decoder::target_t& target,
							const safe_ptr<AVFormatContext>& context,
							double fps,
							const std::wstring& filter) 
		: source_(source)
		, forward_(forward)
		, target_(target)
		, filter_(filter.empty() ? L"copy" : filter)
		, fps_(fps)
		, nb_frames_(0)
		, width_(0)
		, height_(0)
		, is_progressive_(true)
	{
		try
		{
			AVCodec* dec;
			index_ = THROW_ON_ERROR2(av_find_best_stream(context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0), "[video_decoder]");
						
			THROW_ON_ERROR2(tbb_avcodec_open(context->streams[index_]->codec, dec), "[video_decoder]");
								
			codec_context_.reset(context->streams[index_]->codec, tbb_avcodec_close);
		
			CASPAR_LOG(debug) << "[video_decoder] " << context->streams[index_]->codec->codec->long_name;

			// Some files give an invalid time_base numerator, try to fix it.

			fix_meta_data(*context);
			
			fps_ = static_cast<double>(codec_context_->time_base.den) / static_cast<double>(codec_context_->time_base.num);
			nb_frames_ = context->streams[index_]->nb_frames;

			if(double_rate(filter))
				fps_ *= 2;

			width_  = codec_context_->width;
			height_ = codec_context_->height;
		}
		catch(...)
		{
			index_ = THROW_ON_ERROR2(av_find_best_stream(context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0), "[video_decoder]");

			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "[video_decoder] Failed to open video-stream. Running without video.";	
		}

		start();
	}
	
	~implementation()
	{
		agent::wait(this);
	}
	
	virtual void run()
	{
		try
		{
			while(true)
			{
				auto packet = Concurrency::receive(source_, 5000);

				if(packet == eof_packet() || packet == loop_packet() || packet->stream_index != index_)
					Concurrency::send(forward_, packet);
				
				if(packet == eof_packet())				
					break;
								
				if(packet == loop_packet())
				{	
					if(codec_context_)
					{
						if(codec_context_->codec->capabilities & CODEC_CAP_DELAY)
						{
							AVPacket pkt;
							av_init_packet(&pkt);
							pkt.data = nullptr;
							pkt.size = 0;
				
							BOOST_FOREACH(auto& frame1, decode(pkt))
							{
								BOOST_FOREACH(auto& frame2, filter_.execute(frame1))
									Concurrency::send(target_, std::shared_ptr<AVFrame>(frame2));
							}	
						}

						avcodec_flush_buffers(codec_context_.get());
					}
					
					Concurrency::send(target_, loop_video());	
				}
				else if(packet->stream_index == index_)
				{
					if(!codec_context_)
					{
						Concurrency::send(target_, empty_video());	
					}
					else
					{
						while(packet->size > 0)
						{
							BOOST_FOREACH(auto& frame1, decode(*packet))
							{
								BOOST_FOREACH(auto& frame2, filter_.execute(frame1))
									Concurrency::send(target_, std::shared_ptr<AVFrame>(frame2));
							}
						}
					}
				}
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		Concurrency::send(target_, eof_video());
				
		done();
	}

	std::vector<std::shared_ptr<AVFrame>> decode(AVPacket& pkt)
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
			return std::vector<std::shared_ptr<AVFrame>>();

		if(decoded_frame->repeat_pict % 2 > 0)
			CASPAR_LOG(warning) << "[video_decoder]: Field repeat_pict not implemented.";
		
		is_progressive_ = decoded_frame->interlaced_frame == 0;

		return std::vector<std::shared_ptr<AVFrame>>(1 + decoded_frame->repeat_pict/2, decoded_frame);
	}
		
	double fps() const
	{
		return fps_;
	}
};

video_decoder::video_decoder(source_t& source,
							 forward_t& forward,
							 target_t& target,
							 const safe_ptr<AVFormatContext>& context, 
							 double fps, 
							 const std::wstring& filter) 
	: impl_(new implementation(source, forward, target, context, fps, filter))
{
}

double video_decoder::fps() const{return impl_->fps();}
int64_t video_decoder::nb_frames() const{return impl_->nb_frames_;}
size_t video_decoder::width() const{return impl_->width_;}
size_t video_decoder::height() const{return impl_->height_;}
bool video_decoder::is_progressive() const{return impl_->is_progressive_;}

}}