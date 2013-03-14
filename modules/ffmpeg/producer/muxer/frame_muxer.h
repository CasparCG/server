/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include "display_mode.h"

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
	frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, bool thumbnail_mode, const std::wstring& filter = L"");
	
	void push(const std::shared_ptr<AVFrame>& video_frame, int hints = 0);
	void push(const std::shared_ptr<core::audio_buffer>& audio_samples);
	
	bool video_ready() const;
	bool audio_ready() const;

	std::shared_ptr<core::basic_frame> poll();

	uint32_t calc_nb_frames(uint32_t nb_frames) const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}