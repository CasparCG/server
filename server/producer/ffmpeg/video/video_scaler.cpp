#include "../../../stdafx.h"

#include "video_scaler.h"

#include "../../../frame/frame_format.h"
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

// TODO: Remove and do copy right into frame
struct fill_frame
{
	fill_frame(size_t width, size_t height) 
		: frame(avcodec_alloc_frame(), av_free), buffer(static_cast<unsigned char*>(scalable_aligned_malloc(width*height*4, 16)), scalable_aligned_free)
	{	
		avpicture_fill(reinterpret_cast<AVPicture*>(frame.get()), buffer.get(), PIX_FMT_BGRA, width, height);
	}
	const AVFramePtr	frame;
	const std::shared_ptr<unsigned char>		buffer;
};
typedef std::shared_ptr<fill_frame> fill_frame_ptr;

struct video_scaler::implementation : boost::noncopyable
{
	video_packet_ptr execute(const video_packet_ptr video_packet)
	{				
		assert(video_packet);

		if(!sws_context_)
		{
			double param;
			sws_context_.reset(sws_getContext(video_packet->codec_context->width, video_packet->codec_context->height, video_packet->codec_context->pix_fmt, video_packet->format_desc.width, video_packet->format_desc.height, 
												PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
		
		//AVFrame avFrame;	
		//avcodec_get_frame_defaults(avFrame);
		//avpicture_fill(reinterpret_cast<AVPicture*>(&avFrame), video_packet->frame->data(), PIX_FMT_BGRA, video_packet->frameFormat.width, video_packet->frameFormat.height);
		
		fill_frame fill_frame(video_packet->format_desc.width, video_packet->format_desc.height);
		int result = sws_scale(sws_context_.get(), video_packet->decoded_frame->data, video_packet->decoded_frame->linesize, 0, video_packet->codec_context->height, fill_frame.frame->data, fill_frame.frame->linesize);
		video_packet->decoded_frame.reset(); // Free memory
		
		if(video_packet->codec->id == CODEC_ID_DVVIDEO) // Move up one field
		{
			size_t size = video_packet->format_desc.width * video_packet->format_desc.height * 4;
			size_t linesize = video_packet->format_desc.width * 4;
			common::image::copy(video_packet->frame->data(), fill_frame.buffer.get() + linesize, size - linesize);
			common::image::clear(video_packet->frame->data() + size - linesize, linesize);
		}
		else
		{
			 // This copy should be unnecessary. But it seems that when mapping the frame memory to an avframe for scaling there are some artifacts in the picture. See line 59-61.
			common::image::copy(video_packet->frame->data(), fill_frame.buffer.get(), video_packet->frame->size());
		}

		return video_packet;	
	}

private:
	SwsContextPtr sws_context_;
};

video_scaler::video_scaler() : impl_(new implementation()){}
video_packet_ptr video_scaler::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
}}