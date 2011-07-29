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
	frame_muxer(double in_fps, const video_format_desc& format_desc);
	
	void push(const std::shared_ptr<write_frame>& video_frame);
	void push(const std::shared_ptr<std::vector<int16_t>>& audio_samples);
	
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