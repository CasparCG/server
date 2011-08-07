#pragma once

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar { 
	
namespace core {

class write_frame;
class basic_frame;
struct frame_factory;

}

class frame_muxer : boost::noncopyable
{
public:
	frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory);
	
	void push(const std::shared_ptr<AVFrame>& video_frame, int hints = 0);
	void push(const std::shared_ptr<std::vector<int16_t>>& audio_samples);
	
	bool video_ready() const;
	bool audio_ready() const;

	size_t size() const;
	bool empty() const;

	safe_ptr<core::basic_frame> pop();
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}