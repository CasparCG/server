#pragma once

#include "../packet.h"
#include "../../../frame/FrameManager.h"

namespace caspar{ namespace ffmpeg{

class video_transformer : boost::noncopyable
{
public:
	video_transformer();
	video_packet_ptr execute(const video_packet_ptr& video_packet);
	void set_factory(const FrameManagerPtr& factory);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_transformer> video_transformer_ptr;
typedef std::unique_ptr<video_transformer> video_transformer_uptr;

}}