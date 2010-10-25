#pragma once

#include "packet.h"

#include <system_error>
#include "../../frame/frame_fwd.h"

namespace caspar{ namespace ffmpeg{	
	
typedef std::shared_ptr<AVFormatContext> AVFormatContextPtr;

class input : boost::noncopyable
{
public:
	input(const frame_format_desc& format_desc);
	void load(const std::string& filename);
	const std::shared_ptr<AVCodecContext>& get_video_codec_context() const;
	const std::shared_ptr<AVCodecContext>& get_audio_codec_context() const;

	video_packet_ptr get_video_packet();
	audio_packet_ptr get_audio_packet();

	void initialize(const frame_factory_ptr& factory);

	bool is_eof() const;
	void set_loop(bool value);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<input> input_ptr;
typedef std::unique_ptr<input> input_uptr;

	}
}
