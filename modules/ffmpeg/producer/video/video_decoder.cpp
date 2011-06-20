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
#include "pix_fmt.h"

#include "../../ffmpeg_error.h"
#include "../filter/filter.h"

#include <common/memory/memcpy.h>

#include <core/video_format.h>
#include <core/producer/frame/basic_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/frame_factory.h>

#include <tbb/parallel_for.h>

#include <boost/range/algorithm_ext.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libswscale/swscale.h>
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
	
struct video_decoder::implementation : boost::noncopyable
{
	input& input_;
	std::shared_ptr<SwsContext>					sws_context_;
	const std::shared_ptr<core::frame_factory>	frame_factory_;
	AVCodecContext&								codec_context_;
	size_t										frame_number_;

	std::shared_ptr<filter>						filter_;
	size_t										filter_delay_;
	int											eof_count_;

	std::string									filter_str_;

public:
	explicit implementation(input& input, const safe_ptr<core::frame_factory>& frame_factory, const std::string& filter_str) 
		: input_(input)
		, frame_factory_(frame_factory)
		, codec_context_(*input_.get_video_codec_context())
		, frame_number_(0)
		, filter_(filter_str.empty() ? nullptr : new filter(filter_str))
		, filter_delay_(1)
		, filter_str_(filter_str)
		, eof_count_(std::numeric_limits<int>::max())
	{
	}

	std::deque<std::pair<int, safe_ptr<core::write_frame>>> receive()
	{
		std::deque<std::pair<int, safe_ptr<core::write_frame>>> result;
		
		std::shared_ptr<AVPacket> pkt;
		for(int n = 0; n < 32 && result.empty() && input_.try_pop_video_packet(pkt); ++n)	
			boost::range::push_back(result, decode(pkt));

		return result;
	}

	std::deque<std::pair<int, safe_ptr<core::write_frame>>> decode(const std::shared_ptr<AVPacket>& video_packet)
	{			
		if(!video_packet) // eof
			return flush();
		
		std::deque<std::pair<int, safe_ptr<core::write_frame>>> result;
		
		frame_number_ = frame_number_ % eof_count_;
		
		if(filter_)
		{							
			std::shared_ptr<AVFrame> frame;

			tbb::parallel_invoke(
			[&]
			{
				frame = decode_frame(video_packet);
			},
			[&]
			{
				if(filter_->is_ready())
				{				
					boost::range::transform(filter_->poll(), std::back_inserter(result), [&](const safe_ptr<AVFrame>& frame)
					{
						return std::make_pair(frame_number_, make_write_frame(frame));
					});
		
					if(!result.empty())
						++frame_number_;
					else		
						++filter_delay_;
				}
			});		

			if(frame)
				filter_->push(make_safe(frame));
		}
		else
		{
			auto frame = decode_frame(video_packet);
			
			if(frame)
				result.push_back(std::make_pair(frame_number_++, make_write_frame(make_safe(frame))));
		}

		return result;
	}

	std::deque<std::pair<int, safe_ptr<core::write_frame>>> flush()
	{
		std::deque<std::pair<int, safe_ptr<core::write_frame>>> result;

		eof_count_ = frame_number_;

		if(filter_)
			eof_count_ += filter_delay_;		

		avcodec_flush_buffers(&codec_context_);

		return result;
	}
		
	std::shared_ptr<AVFrame> decode_frame(const std::shared_ptr<AVPacket>& video_packet)
	{
		std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		const int errn = avcodec_decode_video2(&codec_context_, decoded_frame.get(), &frame_finished, video_packet.get());
		
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				invalid_operation() <<
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("avcodec_decode_video") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		if(frame_finished == 0)		
			decoded_frame = nullptr;

		return decoded_frame;
	}

	safe_ptr<core::write_frame> make_write_frame(const safe_ptr<AVFrame>& decoded_frame)
	{			
		// We don't know what the filter output might give until we received the first frame. Initialize everything on first frame.
		auto width   = decoded_frame->width;
		auto height  = decoded_frame->height;
		auto pix_fmt = static_cast<PixelFormat>(decoded_frame->format);
		auto desc	 = get_pixel_format_desc(pix_fmt, width, height);
			
		if(desc.pix_fmt == core::pixel_format::invalid)
		{
			CASPAR_VERIFY(!sws_context_); // Initialize only once. Nothing should change while running;
			CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";

			desc = get_pixel_format_desc(PIX_FMT_BGRA, width, height);
			double param;
			sws_context_.reset(sws_getContext(width, height, pix_fmt, width, height, PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
			if(!sws_context_)
				BOOST_THROW_EXCEPTION(operation_failed() <<
									  msg_info("Could not create software scaling context.") << 
									  boost::errinfo_api_function("sws_getContext"));
		}

		auto write = frame_factory_->create_frame(this, desc);
		write->set_is_interlaced(decoded_frame->interlaced_frame != 0);

		if(sws_context_ == nullptr)
		{
			tbb::parallel_for(0, static_cast<int>(desc.planes.size()), 1, [&](int n)
			{
				auto plane            = desc.planes[n];
				auto result           = write->image_data(n).begin();
				auto decoded          = decoded_frame->data[n];
				auto decoded_linesize = decoded_frame->linesize[n];
				
				// Copy line by line since ffmpeg sometimes pads each line.
				tbb::parallel_for(tbb::blocked_range<size_t>(0, static_cast<int>(desc.planes[n].height)), [&](const tbb::blocked_range<size_t>& r)
				{
					for(size_t y = r.begin(); y != r.end(); ++y)
						memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
				});

				write->commit(n);
			});
		}
		else
		{
			// Use sws_scale when provided colorspace has no hw-accel.
			safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
			avcodec_get_frame_defaults(av_frame.get());			
			avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), write->image_data().begin(), PIX_FMT_BGRA, width, height);
		 
			sws_scale(sws_context_.get(), decoded_frame->data, decoded_frame->linesize, 0, height, av_frame->data, av_frame->linesize);	

			write->commit();
		}	

		// Fix field-order if needed. DVVIDEO is in lower field. Make it upper field if needed.
		if(decoded_frame->interlaced_frame)
		{
			switch(frame_factory_->get_video_format_desc().mode)
			{
			case core::video_mode::upper:
				if(!decoded_frame->top_field_first)
					write->get_image_transform().set_fill_translation(0.0f, 0.5/static_cast<double>(height));
				break;
			case core::video_mode::lower:
				if(decoded_frame->top_field_first)
					write->get_image_transform().set_fill_translation(0.0f, -0.5/static_cast<double>(height));
				break;
			}
		}

		return write;
	}
};

video_decoder::video_decoder(input& input, const safe_ptr<core::frame_factory>& frame_factory, const std::string& filter_str) : impl_(new implementation(input, frame_factory, filter_str)){}
std::deque<std::pair<int, safe_ptr<core::write_frame>>> video_decoder::receive(){return impl_->receive();}

}