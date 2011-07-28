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

#include <common/memory/memcpy.h>

#include <core/video_format.h>
#include <core/producer/frame/basic_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/color/color_producer.h>

#include <tbb/task_group.h>

#include <boost/range/algorithm_ext.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
		
struct video_decoder::implementation : boost::noncopyable
{
	const safe_ptr<core::frame_factory>		frame_factory_;
	std::shared_ptr<AVCodecContext>			codec_context_;
	int										index_;
	core::video_mode::type					mode_;

	std::queue<std::shared_ptr<AVPacket>>	packet_buffer_;

	std::unique_ptr<filter>					filter_;
	tbb::task_group							filter_tasks_;

	double									fps_;
public:
	explicit implementation(const std::shared_ptr<AVFormatContext>& context, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter) 
		: frame_factory_(frame_factory)
		, mode_(core::video_mode::invalid)
		, filter_(filter.empty() ? nullptr : new caspar::filter(filter))
		, fps_(frame_factory_->get_video_format_desc().fps)
	{
		AVCodec* dec;
		index_ = av_find_best_stream(context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);

		if(index_ < 0)
			return;
			
		int errn = tbb_avcodec_open(context->streams[index_]->codec, dec);
		if(errn < 0)
			return;
				
		codec_context_.reset(context->streams[index_]->codec, tbb_avcodec_close);

		// Some files give an invalid time_base numerator, try to fix it.
		if(codec_context_ && codec_context_->time_base.num == 1)
			codec_context_->time_base.num = static_cast<int>(std::pow(10.0, static_cast<int>(std::log10(static_cast<float>(codec_context_->time_base.den)))-1));	

		fps_ = static_cast<double>(codec_context_->time_base.den) / static_cast<double>(codec_context_->time_base.num);
		if(double_rate(filter))
			fps_ *= 2;
	}

	~implementation()
	{
		filter_tasks_.cancel();
		filter_tasks_.wait();
	}
		
	void push(const std::shared_ptr<AVPacket>& packet)
	{
		if(!codec_context_)
			return;

		if(packet && packet->stream_index != index_)
			return;

		packet_buffer_.push(packet);
	}

	std::vector<safe_ptr<core::write_frame>> poll()
	{		
		std::vector<safe_ptr<core::write_frame>> result;

		if(!codec_context_)
			result.push_back(make_safe<core::write_frame>(reinterpret_cast<int>(this)));
		else if(!packet_buffer_.empty())
		{
			std::vector<safe_ptr<AVFrame>> av_frames;

			auto packet = std::move(packet_buffer_.front());
			packet_buffer_.pop();
		
			if(packet) // eof
				decode(*packet, av_frames);			
			else
			{
				if(codec_context_->codec->capabilities | CODEC_CAP_DELAY)
				{
					// TODO: This might cause bad performance.
					AVPacket pkt = {0};
					for(int n = 0; n < 8 && decode(pkt, av_frames); ++n){}
				}

				avcodec_flush_buffers(codec_context_.get());
			}

			if(filter_)
			{
				auto av_frames2 = std::move(av_frames);

				filter_tasks_.wait();

				av_frames = filter_->poll();

				filter_tasks_.run([=]
				{
					BOOST_FOREACH(auto& frame, av_frames2)				
						filter_->push(frame);
				});
			}
						
			BOOST_FOREACH(auto& frame, av_frames)
				result.push_back(make_write_frame(this, frame, frame_factory_));
		}
		
		return result;
	}

	bool decode(AVPacket& packet, std::vector<safe_ptr<AVFrame>>& av_frames)
	{
		std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		const int errn = avcodec_decode_video2(codec_context_.get(), decoded_frame.get(), &frame_finished, &packet);
		
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				invalid_operation() <<
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("avcodec_decode_video") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		if(frame_finished != 0)	
			av_frames.push_back(make_safe(decoded_frame));

		return frame_finished != 0;
	}
	
	bool ready() const
	{
		return !codec_context_ || !packet_buffer_.empty();
	}
	
	core::video_mode::type mode()
	{
		if(!codec_context_)
			return frame_factory_->get_video_format_desc().mode;

		return mode_;
	}

	double fps() const
	{
		return fps_;
	}
};

video_decoder::video_decoder(const std::shared_ptr<AVFormatContext>& context, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter) : impl_(new implementation(context, frame_factory, filter)){}
void video_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
std::vector<safe_ptr<core::write_frame>> video_decoder::poll(){return impl_->poll();}
bool video_decoder::ready() const{return impl_->ready();}
core::video_mode::type video_decoder::mode(){return impl_->mode();}
double video_decoder::fps() const{return impl_->fps();}
}