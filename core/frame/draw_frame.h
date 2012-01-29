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

#include "frame_visitor.h"

#include <core/video_format.h>

#include <common/memory/safe_ptr.h>

#include <vector>

namespace caspar { namespace core {
	
class draw_frame
{
public:
	draw_frame();	
	draw_frame(const safe_ptr<draw_frame>& frame);
	draw_frame(safe_ptr<draw_frame>&& frame);
	draw_frame(const std::vector<safe_ptr<draw_frame>>& frames);
	draw_frame(std::vector<safe_ptr<draw_frame>>&& frames);
	draw_frame(const draw_frame& other);
	draw_frame(draw_frame&& other);

	draw_frame& operator=(draw_frame other);
	
	void swap(draw_frame& other);

	const struct frame_transform& get_frame_transform() const;
	struct frame_transform& get_frame_transform();
				
	static safe_ptr<draw_frame> interlace(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2, field_mode mode);
	static safe_ptr<draw_frame> combine(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2);
	static safe_ptr<draw_frame> fill_and_key(const safe_ptr<draw_frame>& fill, const safe_ptr<draw_frame>& key);
		
	static const safe_ptr<draw_frame>& eof()
	{
		static safe_ptr<draw_frame> frame = make_safe<draw_frame>();
		return frame;
	}

	static const safe_ptr<draw_frame>& empty()
	{
		static safe_ptr<draw_frame> frame = make_safe<draw_frame>();
		return frame;
	}

	static const safe_ptr<draw_frame>& late()
	{
		static safe_ptr<draw_frame> frame = make_safe<draw_frame>();
		return frame;
	}
	
	virtual void accept(frame_visitor& visitor);
private:
	struct impl;
	safe_ptr<impl> impl_;
};
	
safe_ptr<draw_frame> disable_audio(const safe_ptr<draw_frame>& frame);

}}