#ifndef _FFMPEG_VIDEO_SCALER2_H_
#define _FFMPEG_VIDEO_SCALER2_H_

#include "..\FFMPEGPacket.h"
#include "FFMPEGVideoDecoder.h"

#include <memory>

#include <tbb/pipeline.h>

#include "..\..\..\frame\Frame.h"
#include "..\..\..\utils\Noncopyable.hpp"

namespace caspar
{
	namespace ffmpeg
	{

class VideoPacketScaler2Filter : public tbb::filter, private utils::Noncopyable
{
public:

	VideoPacketScaler2Filter();

	void* operator()(void* item);
	void finalize(void* item);

	static bool Supports(AVCodecContext* codecContext, const FrameFormatDescription& frameFormat)
	{
		switch(codecContext->pix_fmt)
		{
		//case PIX_FMT_RGB24:
		//	return true;
		default:
			return false;
		}		
	}

private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

	}
}

#endif