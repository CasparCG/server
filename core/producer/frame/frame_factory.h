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

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/range.hpp>

namespace caspar { namespace core {
			
struct frame_factory : boost::noncopyable
{
	typedef boost::iterator_range<uint8_t*> range_type;
	typedef std::vector<range_type> range_vector_type;

	frame_factory(){}
	

	virtual safe_ptr<class device_frame> create_frame(const void* video_stream_tag, 
													 const struct pixel_format_desc& desc, 
													 const std::function<void(const range_vector_type&)>& func, 
													 core::field_mode type = core::field_mode::progressive) = 0;	

	virtual struct video_format_desc get_video_format_desc() const = 0; // nothrow
};

}}