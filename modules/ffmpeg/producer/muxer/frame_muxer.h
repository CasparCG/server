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

#include <common/forward.h>
#include <common/memory.h>

#include <core/mixer/audio/audio_mixer.h>
#include <core/video_format.h>

#include <boost/noncopyable.hpp>

#include <vector>

struct AVFrame;

FORWARD2(caspar, core, struct pixel_format_desc);
FORWARD2(caspar, core, class frame);
FORWARD2(caspar, core, class frame_factory);
FORWARD2(caspar, core, class draw_frame);

namespace caspar { namespace ffmpeg {

class frame_muxer : boost::noncopyable
{
public:
	frame_muxer(double in_fps, const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, const std::wstring& filter = L"");
	
	void push_video(const std::shared_ptr<AVFrame>& frame);
	void push_audio(const std::shared_ptr<AVFrame>& frame);
	
	bool video_ready() const;
	bool audio_ready() const;

	void clear();

	bool empty() const;
	core::draw_frame front() const;
	void pop();

	uint32_t calc_nb_frames(uint32_t nb_frames) const;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}