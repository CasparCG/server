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

#include "FFMPEGVideoDeinterlacer.h"

#include "../FFMPEGPacket.h"
#include "../FFMPEGException.h"

#include "../../../utils/image/Copy.hpp"
#include "../../../utils/image/Image.hpp"

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>

#include <functional>


using namespace std::tr1::placeholders;

namespace caspar
{
	namespace ffmpeg
	{
		
struct VideoPacketDeinterlacerFilter::Implementation : public FFMPEGFilterImpl<FFMPEGVideoPacket>
{
	void* process(FFMPEGVideoPacket* pVideoPacket)
	{				
		std::error_code errc = ffmpeg_make_averror_code(avpicture_deinterlace(reinterpret_cast<AVPicture*>(pVideoPacket->pDecodedFrame.get()), reinterpret_cast<AVPicture*>(pVideoPacket->pDecodedFrame.get()), pVideoPacket->codecContext->pix_fmt, pVideoPacket->codecContext->width, pVideoPacket->codecContext->height));
		if(errc) 
		{
			errc = ffmpeg_make_error_code(ffmpeg_errn::deinterlace_failed);
			return pVideoPacket; // TODO: Unsure if frame can become corrupted by failed deinterlace?
		}
				
		return pVideoPacket;	
	}

	bool is_valid(FFMPEGVideoPacket* pVideoPacket)
	{
		return pVideoPacket->pDecodedFrame != nullptr && pVideoPacket->pDecodedFrame->interlaced_frame;
	}
};

VideoPacketDeinterlacerFilter::VideoPacketDeinterlacerFilter()
	: tbb::filter(parallel), pImpl_(new Implementation())
{
}


void* VideoPacketDeinterlacerFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void VideoPacketDeinterlacerFilter::finalize(void* item)
{
	pImpl_->finalize(item);
}
	}
}