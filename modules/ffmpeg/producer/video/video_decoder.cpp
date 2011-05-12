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

#include <common/memory/memcpy.h>

#include <core/video_format.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/write_frame.h>
#include <core/producer/frame/image_transform.h>

#include <tbb/parallel_for.h>

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
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
	
core::pixel_format::type get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
		case PIX_FMT_GRAY8:		return core::pixel_format::gray;
		case PIX_FMT_BGRA:		return core::pixel_format::bgra;
		case PIX_FMT_ARGB:		return core::pixel_format::argb;
		case PIX_FMT_RGBA:		return core::pixel_format::rgba;
		case PIX_FMT_ABGR:		return core::pixel_format::abgr;
		case PIX_FMT_YUV444P:	return core::pixel_format::ycbcr;
		case PIX_FMT_YUV422P:	return core::pixel_format::ycbcr;
		case PIX_FMT_YUV420P:	return core::pixel_format::ycbcr;
		case PIX_FMT_YUV411P:	return core::pixel_format::ycbcr;
		case PIX_FMT_YUV410P:	return core::pixel_format::ycbcr;
		case PIX_FMT_YUVA420P:	return core::pixel_format::ycbcra;
		default:				return core::pixel_format::invalid;
	}
}

core::pixel_format_desc get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height)
{
	// Get linesizes
	AVPicture dummy_pict;	
	avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

	core::pixel_format_desc desc;
	desc.pix_fmt = get_pixel_format(pix_fmt);
		
	switch(desc.pix_fmt)
	{
	case core::pixel_format::gray:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/4, height, 1));						
			return desc;
		}
	case core::pixel_format::bgra:
	case core::pixel_format::argb:
	case core::pixel_format::rgba:
	case core::pixel_format::abgr:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/4, height, 4));						
			return desc;
		}
	case core::pixel_format::ycbcr:
	case core::pixel_format::ycbcra:
		{		
			// Find chroma height
			size_t size2 = dummy_pict.data[2] - dummy_pict.data[1];
			size_t h2 = size2/dummy_pict.linesize[1];			

			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

			if(desc.pix_fmt == core::pixel_format::ycbcra)						
				desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));	
			return desc;
		}		
	default:		
		desc.pix_fmt = core::pixel_format::invalid;
		return desc;
	}
}

struct video_decoder::implementation : boost::noncopyable
{	
	std::shared_ptr<core::frame_factory> frame_factory_;
	std::shared_ptr<SwsContext> sws_context_;

	AVCodecContext* codec_context_;

	const int width_;
	const int height_;
	const PixelFormat pix_fmt_;
	core::pixel_format_desc desc_;

public:
	explicit implementation(AVCodecContext* codec_context, const safe_ptr<core::frame_factory>& frame_factory) 
		: frame_factory_(frame_factory)
		, codec_context_(codec_context)
		, width_(codec_context_->width)
		, height_(codec_context_->height)
		, pix_fmt_(codec_context_->pix_fmt)
		, desc_(get_pixel_format_desc(pix_fmt_, width_, height_))
	{
		if(desc_.pix_fmt == core::pixel_format::invalid)
		{
			CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";

			desc_ = get_pixel_format_desc(PIX_FMT_BGRA, width_, height_);
			double param;
			sws_context_.reset(sws_getContext(width_, height_, pix_fmt_, width_, height_, PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
			if(!sws_context_)
				BOOST_THROW_EXCEPTION(operation_failed() <<
									  msg_info("Could not create software scaling context.") << 
									  boost::errinfo_api_function("sws_getContext"));
		}
	}
	
	safe_ptr<core::write_frame> execute(void* tag, const aligned_buffer& video_packet)
	{				
		safe_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		const int result = avcodec_decode_video(codec_context_, decoded_frame.get(), &frame_finished, video_packet.data(), video_packet.size());
		
		if(result < 0)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("avcodec_decode_video failed"));
		
		auto write = frame_factory_->create_frame(tag, desc_);
		if(sws_context_ == nullptr)
		{
			tbb::parallel_for(0, static_cast<int>(desc_.planes.size()), 1, [&](int n)
			{
				auto plane            = desc_.planes[n];
				auto result           = write->image_data(n).begin();
				auto decoded          = decoded_frame->data[n];
				auto decoded_linesize = decoded_frame->linesize[n];
				
				// Copy line by line since ffmpeg sometimes pads each line.
				tbb::parallel_for(0, static_cast<int>(desc_.planes[n].height), 1, [&](int y)
				{
					fast_memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
				});
			});
		}
		else
		{
			// Uses sws_scale when we don't support the provided color-space.
			safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
			avcodec_get_frame_defaults(av_frame.get());			
			avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), write->image_data().begin(), PIX_FMT_BGRA, width_, height_);
		 
			sws_scale(sws_context_.get(), decoded_frame->data, decoded_frame->linesize, 0, height_, av_frame->data, av_frame->linesize);	
		}	

		// DVVIDEO is in lower field. Make it upper field if needed.
		if(codec_context_->codec_id == CODEC_ID_DVVIDEO && frame_factory_->get_video_format_desc().mode == core::video_mode::upper)
			write->get_image_transform().set_fill_translation(0.0f, 1.0/static_cast<double>(height_));

		return write;
	}
};

video_decoder::video_decoder(AVCodecContext* codec_context, const safe_ptr<core::frame_factory>& frame_factory) : impl_(new implementation(codec_context, frame_factory)){}
safe_ptr<core::write_frame> video_decoder::execute(void* tag, const aligned_buffer& video_packet){return impl_->execute(tag, video_packet);}

}