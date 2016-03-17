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

#include "StdAfx.h"

#include <common/diagnostics/graph.h>
#include <common/array.h>

#include <core/frame/draw_frame.h>

#include <boost/rational.hpp>

namespace caspar { namespace ffmpeg {

struct ffmpeg_pipeline_backend
{
	virtual ~ffmpeg_pipeline_backend() { }

	virtual void					graph(spl::shared_ptr<caspar::diagnostics::graph> g) = 0;

	virtual void					from_file(std::string filename) = 0;
	virtual void					from_memory_only_audio(int num_channels, int samplerate) = 0;
	virtual void					from_memory_only_video(int width, int height, boost::rational<int> framerate) = 0;
	virtual void					from_memory(int num_channels, int samplerate, int width, int height, boost::rational<int> framerate) = 0;

	virtual void					start_frame(std::uint32_t frame) = 0;
	virtual std::uint32_t			start_frame() const = 0;
	virtual void					length(std::uint32_t frames) = 0;
	virtual std::uint32_t			length() const = 0;
	virtual void					seek(std::uint32_t frame) = 0;
	virtual void					loop(bool value) = 0;
	virtual bool					loop() const = 0;
	virtual std::string				source_filename() const = 0;

	virtual void					vfilter(std::string filter) = 0;
	virtual void					afilter(std::string filter) = 0;
	virtual int						width() const = 0;
	virtual int						height() const = 0;
	virtual boost::rational<int>	framerate() const = 0;
	virtual bool					progressive() const = 0;
	virtual std::uint32_t			last_frame() const = 0;

	virtual void					to_memory(spl::shared_ptr<core::frame_factory> factory, core::video_format_desc format) = 0;
	virtual void					to_file(std::string filename) = 0;
	virtual void					vcodec(std::string codec) = 0;
	virtual void					acodec(std::string codec) = 0;
	virtual void					format(std::string fmt) = 0;

	virtual void					start() = 0;
	virtual bool					try_push_audio(caspar::array<const std::int32_t> data) = 0;
	virtual bool					try_push_video(caspar::array<const std::uint8_t> data) = 0;
	virtual core::draw_frame		try_pop_frame() = 0;
	virtual bool					started() const = 0;
	virtual void					stop() = 0;
};

}}
