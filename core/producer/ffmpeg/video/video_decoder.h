#pragma once

#include "../../../processor/frame_processor_device.h"

#include <memory>

struct AVCodecContext;

namespace caspar { namespace core { namespace ffmpeg {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(AVCodecContext* codec_context);
	safe_ptr<write_frame> execute(const aligned_buffer& video_packet);	
	void initialize(const safe_ptr<frame_processor_device>& frame_processor);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}