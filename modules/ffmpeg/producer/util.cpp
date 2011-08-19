#include "../../stdafx.h"

#include "util.h"

#include "format/flv.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/mixer/write_frame.h>

#include <common/exception/exceptions.h>

#include <tbb/parallel_for.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {

core::video_mode::type get_mode(AVFrame& frame)
{
	if(!frame.interlaced_frame)
		return core::video_mode::progressive;

	return frame.top_field_first ? core::video_mode::upper : core::video_mode::lower;
}

core::pixel_format::type get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
	case CASPAR_PIX_FMT_LUMA:	return core::pixel_format::luma;
	case PIX_FMT_GRAY8:			return core::pixel_format::gray;
	case PIX_FMT_BGRA:			return core::pixel_format::bgra;
	case PIX_FMT_ARGB:			return core::pixel_format::argb;
	case PIX_FMT_RGBA:			return core::pixel_format::rgba;
	case PIX_FMT_ABGR:			return core::pixel_format::abgr;
	case PIX_FMT_YUV444P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV422P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV420P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV411P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV410P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUVA420P:		return core::pixel_format::ycbcra;
	default:					return core::pixel_format::invalid;
	}
}

core::pixel_format_desc get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height)
{
	// Get linesizes
	AVPicture dummy_pict;	
	avpicture_fill(&dummy_pict, nullptr, pix_fmt == CASPAR_PIX_FMT_LUMA ? PIX_FMT_GRAY8 : pix_fmt, width, height);

	core::pixel_format_desc desc;
	desc.pix_fmt = get_pixel_format(pix_fmt);
		
	switch(desc.pix_fmt)
	{
	case core::pixel_format::gray:
	case core::pixel_format::luma:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));						
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

int make_alpha_format(int format)
{
	switch(get_pixel_format(static_cast<PixelFormat>(format)))
	{
	case core::pixel_format::luma:
	case core::pixel_format::gray:
	case core::pixel_format::invalid:
		return format;
	case core::pixel_format::ycbcr:
	case core::pixel_format::ycbcra:
		return CASPAR_PIX_FMT_LUMA;
	default:
	return PIX_FMT_GRAY8;
	}
}

safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints)
{			
	static tbb::concurrent_unordered_map<size_t, tbb::concurrent_queue<std::shared_ptr<SwsContext>>> sws_contexts_;
	
	const auto width  = decoded_frame->width;
	const auto height = decoded_frame->height;
	auto desc		  = get_pixel_format_desc(static_cast<PixelFormat>(decoded_frame->format), width, height);
	
	if(hints & core::frame_producer::ALPHA_HINT)
		desc = get_pixel_format_desc(static_cast<PixelFormat>(make_alpha_format(decoded_frame->format)), width, height);

	if(desc.pix_fmt == core::pixel_format::invalid)
	{
		auto pix_fmt = static_cast<PixelFormat>(decoded_frame->format);

		auto write = frame_factory->create_frame(tag, desc.pix_fmt != core::pixel_format::invalid ? desc : get_pixel_format_desc(PIX_FMT_BGRA, width, height));
		write->set_type(get_mode(*decoded_frame));

		std::shared_ptr<SwsContext> sws_context;

		//CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";

		size_t key = width << 20 | height << 8 | pix_fmt;
			
		auto& pool = sws_contexts_[key];
						
		if(!pool.try_pop(sws_context))
		{
			double param;
			sws_context.reset(sws_getContext(width, height, pix_fmt, width, height, PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
			
		if(!sws_context)
		{
			BOOST_THROW_EXCEPTION(operation_failed() << msg_info("Could not create software scaling context.") << 
									boost::errinfo_api_function("sws_getContext"));
		}	

		// Use sws_scale when provided colorspace has no hw-accel.
		safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
		avcodec_get_frame_defaults(av_frame.get());			
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), write->image_data().begin(), PIX_FMT_BGRA, width, height);
		 
		sws_scale(sws_context.get(), decoded_frame->data, decoded_frame->linesize, 0, height, av_frame->data, av_frame->linesize);	
		pool.push(sws_context);

		write->commit();

		return write;
	}
	else
	{
		auto write = frame_factory->create_frame(tag, desc);
		write->set_type(get_mode(*decoded_frame));

		for(int n = 0; n < static_cast<int>(desc.planes.size()); ++n)
		{
			auto plane            = desc.planes[n];
			auto result           = write->image_data(n).begin();
			auto decoded          = decoded_frame->data[n];
			auto decoded_linesize = decoded_frame->linesize[n];
				
			// Copy line by line since ffmpeg sometimes pads each line.
			tbb::affinity_partitioner ap;
			tbb::parallel_for(tbb::blocked_range<size_t>(0, static_cast<int>(desc.planes[n].height)), [&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t y = r.begin(); y != r.end(); ++y)
					memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
			}, ap);

			write->commit(n);
		}
		
		return write;
	}
}

}