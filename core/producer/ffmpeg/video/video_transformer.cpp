#include "../../../stdafx.h"

#include "video_transformer.h"

#include "../../../frame/frame_format.h"
#include "../../../../common/utility/memory.h"
#include "../../../frame/gpu_frame.h"
#include "../../../frame/gpu_frame.h"
#include "../../../frame/frame_factory.h"

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>
#include <tbb/scalable_allocator.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libswscale/swscale.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar{ namespace ffmpeg{
	
struct video_transformer::implementation : boost::noncopyable
{
	video_packet_ptr execute(const video_packet_ptr video_packet)
	{				
		assert(video_packet);
		size_t width = video_packet->codec_context->width;
		size_t height = video_packet->codec_context->height;
		auto pix_fmt = video_packet->codec_context->pix_fmt;

		if(!sws_context_)
		{
			double param;
			sws_context_.reset(sws_getContext(width, height, pix_fmt, width, height, 
												PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
		
		video_packet->frame = factory_->create_frame(width, height);
		AVFrame av_frame;	
		avcodec_get_frame_defaults(&av_frame);
		avpicture_fill(reinterpret_cast<AVPicture*>(&av_frame), video_packet->frame->data(), PIX_FMT_BGRA, width, height);
		
		sws_scale(sws_context_.get(), video_packet->decoded_frame->data, video_packet->decoded_frame->linesize, 0, height, av_frame.data, av_frame.linesize);
		
		if(video_packet->codec->id == CODEC_ID_DVVIDEO) // Move up one field
			video_packet->frame->translate(0.0f, 1.0/static_cast<double>(video_packet->format_desc.height));
		
		return video_packet;	
	}

	frame_factory_ptr factory_;
	std::shared_ptr<SwsContext> sws_context_;
};

video_transformer::video_transformer() : impl_(new implementation()){}
video_packet_ptr video_transformer::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
void video_transformer::initialize(const frame_factory_ptr& factory){impl_->factory_ = factory; }
}}