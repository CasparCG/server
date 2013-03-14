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

#include <boost/noncopyable.hpp>

#include <common/memory/safe_ptr.h>
#include <common/filesystem/filesystem_monitor.h>

namespace caspar { namespace core {

class ogl_device;
class read_frame;
struct video_format_desc;

typedef std::function<void (
		const safe_ptr<read_frame>& frame,
		const video_format_desc& format_desc,
		const boost::filesystem::wpath& output_file,
		int width,
		int height)> thumbnail_creator;

class thumbnail_generator : boost::noncopyable
{
public:
	thumbnail_generator(
			filesystem_monitor_factory& monitor_factory,
			const boost::filesystem::wpath& media_path,
			const boost::filesystem::wpath& thumbnails_path,
			int width,
			int height,
			const video_format_desc& render_video_mode,
			const safe_ptr<ogl_device>& ogl,
			int generate_delay_millis,
			const thumbnail_creator& thumbnail_creator);
	~thumbnail_generator();
	void generate(const std::wstring& media_file);
	void generate_all();
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}
