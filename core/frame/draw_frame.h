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

#include <common/spl/memory.h>

#include <vector>

namespace caspar { namespace core {
	
class draw_frame
{
public:
	draw_frame();	
	draw_frame(const draw_frame& other);
	draw_frame(draw_frame&& other);
	draw_frame& operator=(draw_frame other);
	virtual ~draw_frame(){}

	draw_frame(spl::shared_ptr<draw_frame> frame);
	draw_frame(std::vector<spl::shared_ptr<draw_frame>> frames);
		
	void swap(draw_frame& other);

	const struct frame_transform& get_frame_transform() const;
	struct frame_transform& get_frame_transform();
				
	static spl::shared_ptr<draw_frame> interlace(const spl::shared_ptr<draw_frame>& frame1, const spl::shared_ptr<draw_frame>& frame2, field_mode mode);
	static spl::shared_ptr<draw_frame> over(const spl::shared_ptr<draw_frame>& frame1, const spl::shared_ptr<draw_frame>& frame2);
	static spl::shared_ptr<draw_frame> mask(const spl::shared_ptr<draw_frame>& fill, const spl::shared_ptr<draw_frame>& key);
	static spl::shared_ptr<draw_frame> silence(const spl::shared_ptr<draw_frame>& frame);
		
	static const spl::shared_ptr<draw_frame>& eof();
	static const spl::shared_ptr<draw_frame>& empty();
	static const spl::shared_ptr<draw_frame>& late();
	
	virtual void accept(frame_visitor& visitor);
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};
	

}}