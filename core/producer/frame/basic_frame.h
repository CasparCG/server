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

#include <boost/range/iterator_range.hpp>

#include <vector>

namespace caspar { namespace core {

struct frame_transform;

class basic_frame
{
public:
	basic_frame();	
	basic_frame(const basic_frame& other);
	basic_frame(basic_frame&& other);
	virtual ~basic_frame(){}

	basic_frame(const safe_ptr<basic_frame>& frame);
	basic_frame(safe_ptr<basic_frame>&& frame);
	basic_frame(const std::vector<safe_ptr<basic_frame>>& frames);
	basic_frame(std::vector<safe_ptr<basic_frame>>&& frames);

	basic_frame& operator=(const basic_frame& other);
	basic_frame& operator=(basic_frame&& other);
	
	void swap(basic_frame& other);

	const frame_transform& get_frame_transform() const;
	frame_transform& get_frame_transform();
				
	static safe_ptr<basic_frame> interlace(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2, field_mode::type mode);
	static safe_ptr<basic_frame> combine(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2);
	static safe_ptr<basic_frame> fill_and_key(const safe_ptr<basic_frame>& fill, const safe_ptr<basic_frame>& key);
		
	static const safe_ptr<basic_frame>& eof()
	{
		static safe_ptr<basic_frame> frame = make_safe<basic_frame>();
		return frame;
	}

	static const safe_ptr<basic_frame>& empty()
	{
		static safe_ptr<basic_frame> frame = make_safe<basic_frame>();
		return frame;
	}

	static const safe_ptr<basic_frame>& late()
	{
		static safe_ptr<basic_frame> frame = make_safe<basic_frame>();
		return frame;
	}
	
	virtual void accept(frame_visitor& visitor);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

safe_ptr<basic_frame> disable_audio(const safe_ptr<basic_frame>& frame);

inline bool is_concrete_frame(const safe_ptr<basic_frame>& frame)
{
	return frame != basic_frame::empty() && frame != basic_frame::eof() && frame != basic_frame::late();
}

inline bool is_concrete_frame(const std::shared_ptr<basic_frame>& frame)
{
	return frame != nullptr && frame.get() != basic_frame::empty().get() && frame.get() != basic_frame::eof().get() && frame.get() != basic_frame::late().get();
}

}}