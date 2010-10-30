#pragma once

#include "../packet.h"

#include "../../../frame/frame_fwd.h"

namespace caspar { namespace core { namespace ffmpeg{

class video_transformer : boost::noncopyable
{
public:
	video_transformer();
	video_packet_ptr execute(const video_packet_ptr& video_packet);	
	void initialize(const frame_factory_ptr& factory);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_transformer> video_transformer_ptr;
typedef std::unique_ptr<video_transformer> video_transformer_uptr;

}}}