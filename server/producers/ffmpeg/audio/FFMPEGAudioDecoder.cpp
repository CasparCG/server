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

#include "FFMPEGAudioDecoder.h"
#include "../FFMPEGException.h"

#include "../../../utils/image/Image.hpp"

#include <queue>

namespace caspar
{
	namespace ffmpeg
	{
		
struct AudioPacketDecoderFilter::Implementation : public FFMPEGFilterImpl<FFMPEGAudioPacket>
{
	Implementation() : discardBytes_(0), currentAudioDataChunkOffset_(0)
	{
		pAudioDecompBuffer_.resize(AudioPacketDecoderFilter::AUDIO_DECOMP_BUFFER_SIZE);
		int alignmentOffset_ = static_cast<unsigned char>(AudioPacketDecoderFilter::ALIGNMENT - (reinterpret_cast<size_t>(&pAudioDecompBuffer_.front()) % AudioPacketDecoderFilter::ALIGNMENT));
		pAlignedAudioDecompAddr_ = &pAudioDecompBuffer_.front() + alignmentOffset_;				
	}
		
	void* process(FFMPEGAudioPacket* pAudioPacket)
	{			
		int maxChunkLength = min(pAudioPacket->audioFrameSize, pAudioPacket->sourceAudioFrameSize);

		int writtenBytes = AudioPacketDecoderFilter::AUDIO_DECOMP_BUFFER_SIZE - AudioPacketDecoderFilter::ALIGNMENT;
		int result = avcodec_decode_audio2(pAudioPacket->codecContext, reinterpret_cast<int16_t*>(pAlignedAudioDecompAddr_), &writtenBytes, pAudioPacket->data, pAudioPacket->size);

		if(result <= 0)
			return pAudioPacket;

		unsigned char* pDecomp = pAlignedAudioDecompAddr_;

		//if there are bytes to discard, do that first
		while(writtenBytes > 0 && discardBytes_ != 0)
		{
			int bytesToDiscard = min(writtenBytes, discardBytes_);
			pDecomp += bytesToDiscard;

			discardBytes_ -= bytesToDiscard;
			writtenBytes -= bytesToDiscard;
		}

		while(writtenBytes > 0)
		{
			//if we're starting on a new chunk, allocate it
			if(pCurrentAudioDataChunk_ == nullptr) 
			{
				pCurrentAudioDataChunk_ = std::make_shared<audio::AudioDataChunk>(pAudioPacket->audioFrameSize);
				currentAudioDataChunkOffset_ = 0;
			}

			//either fill what's left of the chunk or copy all writtenBytes that are left
			int targetLength = min((maxChunkLength - currentAudioDataChunkOffset_), writtenBytes);
			memcpy(pCurrentAudioDataChunk_->GetDataPtr() + currentAudioDataChunkOffset_, pDecomp, targetLength);
			writtenBytes -= targetLength;

			currentAudioDataChunkOffset_ += targetLength;
			pDecomp += targetLength;

			if(currentAudioDataChunkOffset_ >= maxChunkLength) 
			{
				if(maxChunkLength < pAudioPacket->audioFrameSize) 
					memset(pCurrentAudioDataChunk_->GetDataPtr() + maxChunkLength, 0, pAudioPacket->audioFrameSize-maxChunkLength);					
				else if(pAudioPacket->audioFrameSize < pAudioPacket->sourceAudioFrameSize) 
					discardBytes_ = pAudioPacket->sourceAudioFrameSize-pAudioPacket->audioFrameSize;

				pAudioPacket->audioDataChunks.push_back(pCurrentAudioDataChunk_);
				pCurrentAudioDataChunk_.reset();
			}
		}

		return pAudioPacket;
	}
			
	bool is_valid(FFMPEGAudioPacket* pAudioPacket)
	{
		return pAudioPacket->codecContext != nullptr && pAudioPacket->data != nullptr && pAudioPacket->size > 0;
	}

	int									discardBytes_;
		
	std::vector<unsigned char>			pAudioDecompBuffer_;
	unsigned char*						pAlignedAudioDecompAddr_;

	caspar::audio::AudioDataChunkPtr	pCurrentAudioDataChunk_;
	int									currentAudioDataChunkOffset_;
};

AudioPacketDecoderFilter::AudioPacketDecoderFilter()
	: tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}

void* AudioPacketDecoderFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void AudioPacketDecoderFilter::finalize(void* item)
{
	return pImpl_->finalize(item);
}

	}
}