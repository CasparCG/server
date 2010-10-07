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

#include "FFMPEGVideoDecoder.h"
#include "../FFMPEGException.h"

#include "../../../utils/object_pool.h"
#include "../../../utils/image/Image.hpp"

namespace caspar
{
	namespace ffmpeg
	{

struct VideoPacketDecoderFilter::Implementation : public FFMPEGFilterImpl<FFMPEGVideoPacket>
{
	void* process(FFMPEGVideoPacket* pVideoPacket)
	{								
		if(pVideoPacket->codec->id == CODEC_ID_RAWVIDEO) 
			utils::image::Shuffle(pVideoPacket->pFrame->GetDataPtr(), pVideoPacket->data, pVideoPacket->size, 3, 2, 1, 0);
		else
		{
			pVideoPacket->pDecodedFrame = framePool_.construct();

			int frameFinished = 0;
			int result = avcodec_decode_video(pVideoPacket->codecContext, pVideoPacket->pDecodedFrame.get(), &frameFinished, pVideoPacket->data, pVideoPacket->size);
			if(std::error_code errc = ffmpeg_make_averror_code(result)) 						
				pVideoPacket->pDecodedFrame.reset();			
		}

		return pVideoPacket;		
	}

	bool is_valid(FFMPEGVideoPacket* pVideoPacket)
	{
		return pVideoPacket->codecContext != nullptr && pVideoPacket->data != nullptr && pVideoPacket->size > 0;
	}

	utils::object_pool<AVFrame, av_frame_allocator> framePool_; // NOTE: All frames need to be ref released before destruction
};

VideoPacketDecoderFilter::VideoPacketDecoderFilter()
	: tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}


void* VideoPacketDecoderFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void VideoPacketDecoderFilter::finalize(void* item)
{
	pImpl_->finalize(item);
}

	}
}