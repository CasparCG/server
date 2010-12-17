#pragma once

#include <system_error>

#include <tbb/cache_aligned_allocator.h>

struct AVCodecContext;

namespace caspar { namespace core { namespace ffmpeg{	
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class input : boost::noncopyable
{
public:
	input(const std::string& filename);
	const std::shared_ptr<AVCodecContext>& get_video_codec_context() const;
	const std::shared_ptr<AVCodecContext>& get_audio_codec_context() const;

	aligned_buffer get_video_packet();
	aligned_buffer get_audio_packet();

	bool seek(unsigned long long frame);

	bool is_eof() const;
	void set_loop(bool value);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<input> input_ptr;
typedef std::unique_ptr<input> input_uptr;

	}
}}
