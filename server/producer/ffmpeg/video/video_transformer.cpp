#include "../../../stdafx.h"

#include "video_transformer.h"

#include "../../../frame/format.h"
#include "../../../frame/factory.h"
#include "../../../frame/algorithm.h"
#include "../../../frame/system_frame.h"
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

	//	const AVPixFmtDescriptor* desc = &av_pix_fmt_descriptors[PIX_FMT_YUVA420P];

		//memcpy(frame->data(), av_frame->data[0], size);
        
	// size = picture->linesize[0] * height;
		//h2 = (height + (1 << desc->log2_chroma_h) - 1) >> desc->log2_chroma_h;
        //size2 = picture->linesize[1] * h2;
        //picture->data[0] = ptr;
        //picture->data[1] = picture->data[0] + size;
        //picture->data[2] = picture->data[1] + size2;
        //picture->data[3] = picture->data[1] + size2 + size2;
        //return 2 * size + 2 * size2;

namespace caspar { namespace ffmpeg {

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
			sws_context_.reset(sws_getContext(width, height, src_pix_fmt, width, height, dest_pix_fmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr), sws_freeContext);
				
		size_t size = size = avpicture_get_size(dest_pix_fmt, width, height);

		video_packet->frame = factory_->create_frame(width, height); //std::make_shared<system_frame>(width, height, width*height*4);
		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), video_packet->frame->data(), dest_pix_fmt, width, height);
		
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

	void set_factory(const frame_factory_ptr& factory)
	{
		factory_ = factory;
	}

	frame_factory_ptr factory_;
	std::shared_ptr<SwsContext> sws_context_;
};

video_transformer::video_transformer() : impl_(new implementation()){}
video_packet_ptr video_transformer::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
void video_transformer::set_factory(const frame_factory_ptr& factory){ impl_->set_factory(factory); }
}}