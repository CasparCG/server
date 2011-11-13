#pragma once

#include <common/memory/safe_ptr.h>

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>

#include <vector>

struct AVFrame;

namespace caspar { 
	
namespace core {

class write_frame;
class basic_frame;
struct frame_factory;

}

namespace ffmpeg {

class frame_muxer : boost::noncopyable
{
public:
	frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter = L"");
	
	void push(const std::shared_ptr<AVFrame>& video_frame, int hints = 0);
	void push(const std::shared_ptr<core::audio_buffer>& audio_samples);
	
	bool video_ready() const;
	bool audio_ready() const;

	std::shared_ptr<core::basic_frame> poll();

	int64_t calc_nb_frames(int64_t nb_frames) const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}