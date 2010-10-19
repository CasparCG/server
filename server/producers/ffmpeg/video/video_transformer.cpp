#include "../../../stdafx.h"

#include "video_transformer.h"

#include "../../../utils/image/Image.hpp"

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

namespace caspar { namespace ffmpeg {
	
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

struct video_transformer::implementation : boost::noncopyable
{
	video_packet_ptr execute(const video_packet_ptr video_packet)
	{				
		assert(video_packet);

		size_t width = video_packet->codec_context->width;
		size_t height = video_packet->codec_context->height;
		PixelFormat src_pix_fmt = video_packet->codec_context->pix_fmt;
		PixelFormat dest_pix_fmt = PIX_FMT_BGRA;// PIX_FMT_YUVA420P; // PIX_FMT_BGRA;// 

		if(!sws_context_)
			sws_context_.reset(sws_getContext(width, height, src_pix_fmt, factory_->GetFrameFormatDescription().width, factory_->GetFrameFormatDescription().height, dest_pix_fmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr), sws_freeContext);
				
		size_t size = size = avpicture_get_size(dest_pix_fmt, width, height);

		{
			tbb::spin_mutex::scoped_lock lock(mutex_);
			video_packet->frame = factory_->CreateFrame(); 
		}

		fill_frame fill_frame(factory_->GetFrameFormatDescription().width, factory_->GetFrameFormatDescription().height);
		int result = sws_scale(sws_context_.get(), video_packet->decoded_frame->data, video_packet->decoded_frame->linesize, 0, video_packet->codec_context->height, fill_frame.frame->data, fill_frame.frame->linesize);
		video_packet->decoded_frame.reset(); // Free memory
		
		if(video_packet->codec->id == CODEC_ID_DVVIDEO) // Move up one field
		{
			size_t size = factory_->GetFrameFormatDescription().width * factory_->GetFrameFormatDescription().height * 4;
			size_t linesize = factory_->GetFrameFormatDescription().width * 4;
			utils::image::Copy(video_packet->frame->GetDataPtr(), fill_frame.buffer.get() + linesize, size - linesize);
			utils::image::Clear(video_packet->frame->GetDataPtr() + size - linesize, linesize);
		}
		else
		{
			 // This copy should be unnecessary. But it seems that when mapping the frame memory to an avframe for scaling there are some artifacts in the picture. See line 59-61.
			utils::image::Copy(video_packet->frame->GetDataPtr(), fill_frame.buffer.get(), video_packet->frame->GetDataSize());
		}
		
		return video_packet;	
	}

	void set_factory(const FrameManagerPtr& factory)
	{
		tbb::spin_mutex::scoped_lock lock(mutex_);
		factory_ = factory;
	}

	tbb::spin_mutex mutex_;
	FrameManagerPtr factory_;
	std::shared_ptr<SwsContext> sws_context_;
};

video_transformer::video_transformer() : impl_(new implementation()){}
video_packet_ptr video_transformer::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
void video_transformer::set_factory(const FrameManagerPtr& factory){ impl_->set_factory(factory); }
}}