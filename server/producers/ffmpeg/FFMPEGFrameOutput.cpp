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
 
#include "../../stdafx.h"

#include "FFMPEGFrameOutput.h"

#include "FFMPEGProducer.h"
#include "FFMPEGPacket.h"
#include "video/FFMPEGVideoDecoder.h"
#include "audio/FFMPEGAudioDecoder.h"

#include "../../utils/image/Image.hpp"

#include <tbb/concurrent_queue.h>

#include <deque>

namespace caspar
{
	namespace ffmpeg
	{

struct FrameOutputFilter::Implementation : public FFMPEGFilterImpl<FFMPEGPacket>
{
	void* process(FFMPEGPacket* pPacket)
	{		
		if(pPacket->type == FFMPEGVideoPacket::packet_type)
		{
			FFMPEGVideoPacket* pVideoPacket = static_cast<FFMPEGVideoPacket*>(pPacket);
			if(pVideoPacket->pFrame != nullptr)
				videoFrameQueue_.push_back(new FFMPEGVideoFrame(pVideoPacket));
		}
		else if(pPacket->type == FFMPEGAudioPacket::packet_type)
		{
			FFMPEGAudioPacket* pAudioPacket = static_cast<FFMPEGAudioPacket*>(pPacket);	
			if(pAudioPacket->audioDataChunks.size() > 0)
				audioChunkQueue_.insert(audioChunkQueue_.end(), pAudioPacket->audioDataChunks.begin(), pAudioPacket->audioDataChunks.end());
		}

		delete pPacket;
					
		while(!videoFrameQueue_.empty() && (!audioChunkQueue_.empty() || !videoFrameQueue_.front()->hasAudio))
		{
			if(videoFrameQueue_.front()->hasAudio)
			{
				videoFrameQueue_.front()->pFrame->AddAudioDataChunk(audioChunkQueue_.front());
				audioChunkQueue_.pop_front();
			}
				
			FFMPEGVideoFrame* pVideoFrame = videoFrameQueue_.front();
			videoFrameQueue_.pop_front();
			return pVideoFrame;
		}
				
		return nullptr;
	}
	
	std::deque<FFMPEGVideoFrame*> videoFrameQueue_;
	std::deque<audio::AudioDataChunkPtr> audioChunkQueue_;
};

FrameOutputFilter::FrameOutputFilter()
	: tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}

void* FrameOutputFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}


	}
}