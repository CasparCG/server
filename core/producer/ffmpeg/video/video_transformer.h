#pragma once

#include "../../../processor/frame_processor_device.h"

#include <memory>

struct AVCodecContext;
struct AVFrame;

namespace caspar { namespace core { namespace ffmpeg{

class video_transformer : boost::noncopyable
{
public:
	video_transformer(AVCodecContext* codec_context);
	safe_ptr<write_frame> execute(const safe_ptr<AVFrame>& video_packet);	
	void initialize(const safe_ptr<frame_processor_device>& frame_processor);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_transformer> video_transformer_ptr;
typedef std::unique_ptr<video_transformer> video_transformer_uptr;

}}}