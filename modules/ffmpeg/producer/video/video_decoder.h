#pragma once

#include <common/memory/safe_ptr.h>

#include <mixer/frame_mixer_device.h>

struct AVCodecContext;

namespace caspar {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(AVCodecContext* codec_context, const safe_ptr<core::frame_factory>& frame_factory);
	safe_ptr<core::write_frame> execute(void* tag, const aligned_buffer& video_packet);	
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}