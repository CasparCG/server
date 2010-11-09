#include "../../../stdafx.h"

#include "video_transformer.h"

#include "../../../video/video_format.h"
#include "../../../../common/utility/memory.h"
#include "../../../processor/frame.h"
#include "../../../processor/frame.h"
#include "../../../processor/frame_processor_device.h"

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>
#include <tbb/scalable_allocator.h>

#include <unordered_map>

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


namespace caspar { namespace core { namespace ffmpeg{
	
pixel_format::type get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
		case PIX_FMT_BGRA:		return pixel_format::bgra;
		case PIX_FMT_ARGB:		return pixel_format::argb;
		case PIX_FMT_RGBA:		return pixel_format::rgba;
		case PIX_FMT_ABGR:		return pixel_format::abgr;
		case PIX_FMT_YUV444P:	return pixel_format::ycbcr;
		case PIX_FMT_YUV422P:	return pixel_format::ycbcr;
		case PIX_FMT_YUV420P:	return pixel_format::ycbcr;
		case PIX_FMT_YUV411P:	return pixel_format::ycbcr;
		case PIX_FMT_YUV410P:	return pixel_format::ycbcr;
		case PIX_FMT_YUVA420P:	return pixel_format::ycbcra;
		default:				return pixel_format::invalid;
	}
}

struct video_transformer::implementation : boost::noncopyable
{
	implementation(AVCodecContext* codec_context) : sw_warning_(false), codec_context_(codec_context){}

	~implementation()
	{
		if(frame_processor_)
			frame_processor_->release_tag(this);
	}

	frame_ptr execute(const std::shared_ptr<AVFrame>& decoded_frame)
	{				
		const int width = codec_context_->width;
		const int height = codec_context_->height;
		const auto pix_fmt = codec_context_->pix_fmt;

		frame_ptr transformed_frame;

		switch(pix_fmt)
		{
		case PIX_FMT_BGRA:
		case PIX_FMT_ARGB:
		case PIX_FMT_RGBA:
		case PIX_FMT_ABGR:
			{
				transformed_frame = frame_processor_->create_frame(width, height, this);
				tbb::parallel_for(0, height, 1, [&](int y)
				{
					common::aligned_memcpy(
						transformed_frame->data()+y*width*4, 
						decoded_frame->data[0] + y*decoded_frame->linesize[0], 
						width*4); 
				});
				transformed_frame->pix_fmt(get_pixel_format(pix_fmt));
						
				break;
			}
		case PIX_FMT_YUV444P:
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUV420P:
		case PIX_FMT_YUV411P:
		case PIX_FMT_YUV410P:
		case PIX_FMT_YUVA420P:
			{			
				// Get linesizes
				AVPicture dummy_pict;	
				avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);
			
				// Find chroma height
				size_t size2 = dummy_pict.data[2] - dummy_pict.data[1];
				size_t h2 = size2/dummy_pict.linesize[1];

				pixel_format_desc desc;
				desc.planes[0] = pixel_format_desc::plane(dummy_pict.linesize[0], height, 1);
				desc.planes[1] = pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1);
				desc.planes[2] = pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1);

				if(pix_fmt == PIX_FMT_YUVA420P)						
					desc.planes[3] = pixel_format_desc::plane(dummy_pict.linesize[3], height, 1);				

				desc.pix_fmt = get_pixel_format(pix_fmt);
				transformed_frame = frame_processor_->create_frame(desc, this);

				tbb::parallel_for(0, static_cast<int>(desc.planes.size()), 1, [&](int n)
				{
					if(desc.planes[n].size == 0)
						return;

					tbb::parallel_for(0, static_cast<int>(desc.planes[n].height), 1, [&](int y)
					{
						memcpy(
							transformed_frame->data(n)+y*dummy_pict.linesize[n], 
							decoded_frame->data[n] + y*decoded_frame->linesize[n], 
							dummy_pict.linesize[n]);
					});
				});
				break;
			}		
		default:	
			{
				if(!sw_warning_)
				{
					CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";
					sw_warning_ = true;
				}
				transformed_frame = frame_processor_->create_frame(width, height, this);

				AVFrame av_frame;	
				avcodec_get_frame_defaults(&av_frame);
				avpicture_fill(reinterpret_cast<AVPicture*>(&av_frame), transformed_frame->data(), PIX_FMT_BGRA, width, height);

				if(!sws_context_)
				{
					double param;
					sws_context_.reset(sws_getContext(width, height, pix_fmt, width, height, PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
				}		
		 
				sws_scale(sws_context_.get(), decoded_frame->data, decoded_frame->linesize, 0, height, av_frame.data, av_frame.linesize);		
			}
		}

		// TODO:
		if(codec_context_->codec_id == CODEC_ID_DVVIDEO) // Move up one field
			transformed_frame->translate(0.0f, 1.0/static_cast<double>(frame_processor_->get_video_format_desc().height));
		
		return transformed_frame;
	}

	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		frame_processor_ = frame_processor;
	}

	frame_processor_device_ptr frame_processor_;
	std::shared_ptr<SwsContext> sws_context_;
	bool sw_warning_;

	AVCodecContext* codec_context_;
};

video_transformer::video_transformer(AVCodecContext* codec_context) : impl_(new implementation(codec_context)){}
frame_ptr video_transformer::execute(const std::shared_ptr<AVFrame>& video_packet){return impl_->execute(video_packet);}
void video_transformer::initialize(const frame_processor_device_ptr& frame_processor){impl_->initialize(frame_processor); }
}}}