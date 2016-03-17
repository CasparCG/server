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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <common/memory.h>
#include <common/array.h>

#include <core/fwd.h>

#include <boost/rational.hpp>

#include <string>
#include <functional>
#include <cstdint>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace ffmpeg {

struct ffmpeg_pipeline_backend;

class ffmpeg_pipeline
{
public:
	ffmpeg_pipeline();

	ffmpeg_pipeline			graph(spl::shared_ptr<caspar::diagnostics::graph> g);

	ffmpeg_pipeline			from_file(std::string filename);
	ffmpeg_pipeline			from_memory_only_audio(int num_channels, int samplerate);
	ffmpeg_pipeline			from_memory_only_video(int width, int height, boost::rational<int> framerate);
	ffmpeg_pipeline			from_memory(int num_channels, int samplerate, int width, int height, boost::rational<int> framerate);

	ffmpeg_pipeline			start_frame(std::uint32_t frame);
	std::uint32_t			start_frame() const;
	ffmpeg_pipeline			length(std::uint32_t frames);
	std::uint32_t			length() const;
	ffmpeg_pipeline			seek(std::uint32_t frame);
	ffmpeg_pipeline			loop(bool value);
	bool					loop() const;
	std::string				source_filename() const;

	ffmpeg_pipeline			vfilter(std::string filter);
	ffmpeg_pipeline			afilter(std::string filter);
	int						width() const;
	int						height() const;
	boost::rational<int>	framerate() const;
	bool					progressive() const;

	ffmpeg_pipeline			to_memory(spl::shared_ptr<core::frame_factory> factory, core::video_format_desc format);
	ffmpeg_pipeline			to_file(std::string filename);
	ffmpeg_pipeline			vcodec(std::string codec);
	ffmpeg_pipeline			acodec(std::string codec);
	ffmpeg_pipeline			format(std::string fmt);

	ffmpeg_pipeline			start();
	bool					try_push_audio(caspar::array<const std::int32_t> data);
	bool					try_push_video(caspar::array<const std::uint8_t> data);
	core::draw_frame		try_pop_frame();
	std::uint32_t			last_frame() const;
	bool					started() const;
	void					stop();

private:
	std::shared_ptr<ffmpeg_pipeline_backend> impl_;
};

}}
