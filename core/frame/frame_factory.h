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

#include "data_frame.h"

#include <common/spl/memory.h>

#include <core/video_format.h>

namespace caspar { namespace core {
			
class frame_factory : boost::noncopyable
{
	frame_factory(const frame_factory&);
	frame_factory& operator=(const frame_factory&);
public:
	/// Static Members

	//Constructors

	frame_factory(){}
	virtual ~frame_factory(){}

	/// Methods

	virtual spl::unique_ptr<class data_frame>	create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) = 0;	
	spl::unique_ptr<class data_frame>			create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc)
	{
		auto format_desc = video_format_desc();
		return create_frame(video_stream_tag, desc, format_desc.fps, format_desc.field_mode);
	}

	/// Properties

	virtual struct video_format_desc video_format_desc() const = 0; 
};

}}