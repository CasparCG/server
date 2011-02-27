#pragma once

#include <common/diagnostics/graph.h>
#include <common/utility/printable.h>

#include <tbb/cache_aligned_allocator.h>

#include <memory>
#include <string>

struct AVCodecContext;

namespace caspar { namespace core { namespace ffmpeg{	
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class input : boost::noncopyable
{
public:
	explicit input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, const printer& parent_printer);
	const std::shared_ptr<AVCodecContext>& get_video_codec_context() const;
	const std::shared_ptr<AVCodecContext>& get_audio_codec_context() const;

	aligned_buffer get_video_packet();
	aligned_buffer get_audio_packet();

	bool is_eof() const;
	double fps() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

	}
}}
