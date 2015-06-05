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

#include <common/memory.h>
#include <common/filesystem_monitor.h>

namespace caspar { namespace core {

class ogl_device;
class read_frame;
struct video_format_desc;
struct media_info_repository;

typedef std::function<void (
		const class const_frame& frame,
		const video_format_desc& format_desc,
		const boost::filesystem::path& output_file,
		int width,
		int height)> thumbnail_creator;

class thumbnail_generator : boost::noncopyable
{
public:
	thumbnail_generator(
			filesystem_monitor_factory& monitor_factory,
			const boost::filesystem::path& media_path,
			const boost::filesystem::path& thumbnails_path,
			int width,
			int height,
			const video_format_desc& render_video_mode,
			std::unique_ptr<class image_mixer> image_mixer,
			int generate_delay_millis,
			const thumbnail_creator& thumbnail_creator,
			spl::shared_ptr<media_info_repository> media_info_repo,
			bool mipmap);
	~thumbnail_generator();
	void generate(const std::wstring& media_file);
	void generate_all();
private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}
