#pragma once

#include <tbb/cache_aligned_allocator.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <vector>

struct AVCodecContext;

namespace caspar {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class audio_decoder : boost::noncopyable
{
public:
	explicit audio_decoder(AVCodecContext* codec_context, double fps);
	std::vector<std::vector<short>> execute(const aligned_buffer& audio_packet);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}