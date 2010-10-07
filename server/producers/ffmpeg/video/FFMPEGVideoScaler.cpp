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
 
#include "../../../stdafx.h"

#include "FFMPEGVideoScaler.h"

#include "../FFMPEGException.h"

#include "../../../utils/image/Copy.hpp"
#include "../../../utils/image/Image.hpp"
#include "../../../utils/object_pool.h"

#include <functional>

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libswscale/swscale.h>
}

using namespace std::tr1::placeholders;

typedef std::shared_ptr<SwsContext> SwsContextPtr;

namespace caspar
{
	namespace ffmpeg
	{

// TODO: Remove and do copy right into frame
struct FillFrame
{
	FillFrame(size_t width, size_t height)
	{
		pFrame.reset(avcodec_alloc_frame(), av_free);		
		if(pFrame == nullptr)
			throw ffmpeg_error(std::make_error_code(std::generic_errno::not_enough_memory));

		size_t frameSize = width*height*4;
		pBuffer.reset(allocator_.allocate(frameSize), [=](unsigned char* item){allocator_.deallocate(item, frameSize);});
		avpicture_fill(reinterpret_cast<AVPicture*>(pFrame.get()), pBuffer.get(), PIX_FMT_BGRA, width, height);
	}
	AVFramePtr	pFrame;
	BytePtr		pBuffer;

	static tbb::cache_aligned_allocator<unsigned char> allocator_;
};
tbb::cache_aligned_allocator<unsigned char> FillFrame::allocator_;
typedef std::shared_ptr<FillFrame> FillFramePtr;

struct VideoPacketScalerFilter::Implementation : public FFMPEGFilterImpl<FFMPEGVideoPacket>
{
	void* process(FFMPEGVideoPacket* pVideoPacket)
	{				
		auto pSWSContext = GetSWSContext(pVideoPacket->codecContext, pVideoPacket->frameFormat);
		
		//AVFrame avFrame;	
		//avpicture_fill(reinterpret_cast<AVPicture*>(&avFrame), pVideoPacket->pFrame->GetDataPtr(), PIX_FMT_BGRA, pVideoPacket->frameFormat.width, pVideoPacket->frameFormat.height);
		
		auto pAVFrame = framePool_.construct(pVideoPacket->frameFormat.width, pVideoPacket->frameFormat.height);
		int result = sws_scale(pSWSContext.get(), pVideoPacket->pDecodedFrame->data, pVideoPacket->pDecodedFrame->linesize, 0, codecContext_->height, pAVFrame->pFrame->data, pAVFrame->pFrame->linesize);
		pVideoPacket->pDecodedFrame.reset(); // Free memory
		
		if(pVideoPacket->codec->id == CODEC_ID_DVVIDEO) // Move up one field
		{
			size_t size = pVideoPacket->frameFormat.width * pVideoPacket->frameFormat.height * 4;
			size_t linesize = pVideoPacket->frameFormat.width * 4;
			utils::image::Copy(pVideoPacket->pFrame->GetDataPtr(), pAVFrame->pBuffer.get() + linesize, size - linesize);
			utils::image::Clear(pVideoPacket->pFrame->GetDataPtr() + size - linesize, linesize);
		}
		else
		{
			 // This copy should be unnecessary. But it seems that when mapping the frame memory to an avframe for scaling there are some artifacts in the picture. 
			utils::image::Copy(pVideoPacket->pFrame->GetDataPtr(), pAVFrame->pBuffer.get(), pVideoPacket->pFrame->GetDataSize());
		}

		return pVideoPacket;	
	}

	SwsContextPtr GetSWSContext(AVCodecContext* codecContext, const FrameFormatDescription& frameFormat)
	{
		if(!pSWSContext_ || codecContext_ != codecContext || frameFormat_.width != frameFormat.width || frameFormat_.height != frameFormat.height)
		{
			frameFormat_ = frameFormat;
			codecContext_ = codecContext;
			double param;
			pSWSContext_.reset(sws_getContext(codecContext_->width, codecContext_->height, codecContext_->pix_fmt, frameFormat_.width, frameFormat_.height, 
												PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
			if(pSWSContext_ == nullptr)
				throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errn::sws_failure));
		}
		return pSWSContext_;
	}

	bool is_valid(FFMPEGVideoPacket* pVideoPacket)
	{
		return pVideoPacket->codecContext != nullptr &&  pVideoPacket->pDecodedFrame != nullptr;
	}

private:
	FrameFormatDescription frameFormat_;
	AVCodecContext* codecContext_;
	SwsContextPtr pSWSContext_;
	utils::object_pool<FillFrame> framePool_;
};

VideoPacketScalerFilter::VideoPacketScalerFilter()
	: tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}

void* VideoPacketScalerFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void VideoPacketScalerFilter::finalize(void* item)
{
	pImpl_->finalize(item);
}

	}
}



// 	utils::image::Copy(pVideoPacket->pFrame->GetDataPtr(), pVideoPacket->pFrame->GetDataPtr(), size);