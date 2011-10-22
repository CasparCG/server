#pragma once

#include <common/memory/safe_ptr.h>

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>

#include <agents.h>
#include <semaphore.h>

#include <vector>

struct AVFrame;

namespace caspar { 
	
namespace core {

class write_frame;
class basic_frame;
struct frame_factory;

}

namespace ffmpeg {

class frame_muxer2 : boost::noncopyable
{
public:
	
	typedef Concurrency::ISource<std::shared_ptr<AVFrame>>				video_source_t;
	typedef Concurrency::ISource<std::shared_ptr<core::audio_buffer>>	audio_source_t;
	typedef Concurrency::ITarget<safe_ptr<core::basic_frame>>			target_t;

	frame_muxer2(video_source_t* video_source,
				 audio_source_t* audio_source, 
				 target_t& target,
				 double in_fps, 
				 const safe_ptr<core::frame_factory>& frame_factory);
	
	int64_t calc_nb_frames(int64_t nb_frames) const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}