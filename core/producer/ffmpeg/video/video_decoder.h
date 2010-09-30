#pragma once

#include "../packet.h"

namespace caspar{ namespace ffmpeg{

typedef std::tr1::shared_ptr<AVCodecContext> AVCodecContextPtr;

class video_decoder : boost::noncopyable
{
public:
	video_decoder();
	video_packet_ptr execute(const video_packet_ptr& video_packet);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_decoder> video_decoder_ptr;
typedef std::unique_ptr<video_decoder> video_decoder_uptr;

	}
}