#pragma once

#include <common/memory/safe_ptr.h>

#include <mixer/frame_mixer_device.h>

struct AVCodecContext;

namespace caspar { namespace core { namespace ffmpeg {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(AVCodecContext* codec_context);
	safe_ptr<write_frame> execute(const aligned_buffer& video_packet);	
	void initialize(const safe_ptr<frame_factory>& frame_factory);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}}