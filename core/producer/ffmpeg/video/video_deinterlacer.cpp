#include "../../../stdafx.h"

#include "video_deinterlacer.h"

#include "../packet.h"

#include "../../../../common/utility/memory.h"

#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
#include <tbb/mutex.h>
#include <tbb/concurrent_queue.h>

using namespace std::tr1::placeholders;

namespace caspar
{
	namespace ffmpeg
	{
		
//struct VideoPacketDeinterlacerFilter::Implementation
//{
//	void* process(video_packet* pVideoPacket)
//	{				
//		avpicture_deinterlace(reinterpret_cast<AVPicture*>(pVideoPacket->pDecodedFrame.get()), reinterpret_cast<AVPicture*>(pVideoPacket->pDecodedFrame.get()), pVideoPacket->codecContext->pix_fmt, pVideoPacket->codecContext->width, pVideoPacket->codecContext->height);
//		return pVideoPacket;	
//	}
//
//	bool is_valid(video_packet* pVideoPacket)
//	{
//		return pVideoPacket->pDecodedFrame != nullptr && pVideoPacket->pDecodedFrame->interlaced_frame;
//	}
//};
//
//VideoPacketDeinterlacerFilter::VideoPacketDeinterlacerFilter()
//	: tbb::filter(parallel), pImpl_(new Implementation())
//{
//}
//
//
//void* VideoPacketDeinterlacerFilter::operator()(void* item)
//{
//	return (*pImpl_)(item);
//}
//
//void VideoPacketDeinterlacerFilter::finalize(void* item)
//{
//	pImpl_->finalize(item);
//}
	}
}