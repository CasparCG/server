#pragma once

#include <tbb/cache_aligned_allocator.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <vector>

struct AVCodecContext;
struct AVFrame;

namespace caspar { namespace core { namespace ffmpeg{
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class video_decoder : boost::noncopyable
{
public:
	video_decoder(AVCodecContext* codec_context);
	safe_ptr<AVFrame> execute(const aligned_buffer& video_packet);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<video_decoder> video_decoder_ptr;
typedef std::unique_ptr<video_decoder> video_decoder_uptr;

	}
}}