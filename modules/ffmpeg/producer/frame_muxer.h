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

#include <agents.h>

namespace ffmpeg {

class frame_muxer : boost::noncopyable
{
public:
	typedef Concurrency::ITarget<safe_ptr<core::basic_frame>> target_t;

	frame_muxer(target_t& target, double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter_str);
	
	void push(const safe_ptr<AVFrame>& video_frame, int hints = 0);
	void push(const safe_ptr<core::audio_buffer>& audio_samples);
	
	bool need_video() const;
	bool need_audio() const;	

	int64_t calc_nb_frames(int64_t nb_frames) const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}