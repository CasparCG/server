#include "../../../stdafx.h"

#include "video_transformer.h"

#include "../../../frame/format.h"
#include "../../../../common/image/image.h"

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

typedef std::shared_ptr<SwsContext> SwsContextPtr;

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
			sws_context_.reset(sws_getContext(width, height, pix_fmt, width, height, PIX_FMT_BGRA, SWS_FAST_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
		
		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avcodec_get_frame_defaults(av_frame.get());
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), video_packet->frame->data(), PIX_FMT_BGRA, width, height);
		
		int result = sws_scale(sws_context_.get(), video_packet->decoded_frame->data, video_packet->decoded_frame->linesize, 0, height, av_frame->data, av_frame->linesize);
		video_packet->decoded_frame.reset(); // Free memory
		
		if(video_packet->codec->id == CODEC_ID_DVVIDEO) // Move up one field
		{
			size_t size = video_packet->frame->size();
			size_t linesize = video_packet->frame->width() * 4;
			common::image::copy(video_packet->frame->data(), video_packet->frame->data() + linesize, size - linesize);
			common::image::clear(video_packet->frame->data() + size - linesize, linesize);
		}


		return video_packet;	
	}

private:
	SwsContextPtr sws_context_;
};

video_transformer::video_transformer() : impl_(new implementation()){}
video_packet_ptr video_transformer::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
}}