#pragma once

#include "../packet.h"

namespace caspar{ namespace ffmpeg{

class video_scaler : boost::noncopyable
{
public:
	video_scaler();
	video_packet_ptr execute(const video_packet_ptr& video_packet);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_scaler> video_scaler_ptr;
typedef std::unique_ptr<video_scaler> video_scaler_uptr;

}}