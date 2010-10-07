#include "..\..\..\stdafx.h"

#include "FFMPEGVideoScaler2.h"
#include "..\FFMPEGException.h"

#include "..\..\..\utils\image\Copy.hpp"
#include "..\..\..\utils\image\Image.hpp"
#include "..\..\..\utils\ObjectPool.h"

#include <functional>

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>
#include <ffmpeg/swscale.h>

#include "../../../utils/Types.hpp"

using namespace std::tr1::placeholders;

typedef std::shared_ptr<SwsContext> SwsContextPtr;

namespace caspar
{
	namespace ffmpeg
	{

void Scale4_Bilinear_REF(void* dest, const void* source, size_t destWidth, size_t destHeight, size_t srcWidth, size_t srcHeight)
{
	const u8* source8 = reinterpret_cast<const u8*>(source);
	u8* dest8 = reinterpret_cast<u8*>(dest);
}
				
struct VideoPacketScaler2Filter::Implementation
{
	void* operator()(void* item)
	{		
		if(item == nullptr)
			return nullptr;

		FFMPEGPacket* pPacket = static_cast<FFMPEGPacket*>(item);

		if(pPacket->type != FFMPEGVideoPacket::packet_type)
			return item;

		FFMPEGVideoPacket* pVideoPacket = static_cast<FFMPEGVideoPacket*>(pPacket); 

		if(!is_valid(pVideoPacket)) 
			return pVideoPacket;

		return Scale(pVideoPacket);
	}

	void* Scale(FFMPEGVideoPacket* pVideoPacket)
	{		
		return pVideoPacket;		
	}

	void finalize(void* item)
	{
		delete static_cast<FFMPEGPacket*>(item);
	}	
	
	bool is_valid(FFMPEGVideoPacket* pVideoPacket)
	{
		return pVideoPacket->pDecodedFrame != nullptr && 
			   pVideoPacket->pFrame == nullptr &&
			   pVideoPacket->codecContext != nullptr && 
			   pVideoPacket->codec != nullptr && 
			   Supports(pVideoPacket->codecContext, pVideoPacket->frameFormat);
	}

private:
	FrameFormatDescription frameFormat_;
	AVCodecContext* codecContext_;
};

VideoPacketScaler2Filter::VideoPacketScaler2Filter()
	: tbb::filter(parallel), pImpl_(new Implementation())
{
}

void* VideoPacketScaler2Filter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void VideoPacketScaler2Filter::finalize(void* item)
{
	pImpl_->finalize(item);
}

	}
}