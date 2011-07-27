#pragma once

#include "../video_format.h"

#include <common/memory/safe_ptr.h>

#include <vector>

namespace caspar { namespace core {

class write_frame;
class basic_frame;

class frame_muxer
{
public:
	frame_muxer(double in_fps, const video_mode::type out_mode, double out_fps);

	void push(const safe_ptr<write_frame>& video_frame);
	void push(const std::vector<int16_t>& audio_chunk);
	
	size_t video_frames() const;
	size_t audio_chunks() const;

	size_t size() const;
	bool empty() const;

	safe_ptr<basic_frame> pop();
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}