#pragma once

#include "util.h"

#include <common/memory/safe_ptr.h>
#include <common/concurrency/governor.h>

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>

#include <agents.h>

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
	
	typedef	std::pair<safe_ptr<AVFrame>, ticket_t>				video_source_element_t;
	typedef	std::pair<safe_ptr<core::audio_buffer>, ticket_t>	audio_source_element_t;

	typedef Concurrency::ISource<video_source_element_t>		video_source_t;
	typedef Concurrency::ISource<audio_source_element_t>		audio_source_t;
								 
	frame_muxer2(video_source_t* video_source,
				 audio_source_t* audio_source, 
				 double in_fps, 
				 const safe_ptr<core::frame_factory>& frame_factory,
				 const std::wstring& filter = L"");
	
	safe_ptr<core::basic_frame> receive();

	int64_t calc_nb_frames(int64_t nb_frames) const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}