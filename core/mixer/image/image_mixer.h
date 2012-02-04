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

#include "blend_modes.h"

#include <common/forward.h>
#include <common/spl/memory.h>

#include <core/frame/frame_visitor.h>

FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {
	
struct image_mixer : public frame_visitor
{
	virtual ~image_mixer(){}
	
	virtual void begin(class draw_frame& frame) = 0;
	virtual void visit(class write_frame& frame) = 0;
	virtual void end() = 0;

	virtual void begin_layer(blend_mode blend_mode) = 0;
	virtual void end_layer() = 0;
		
	virtual boost::unique_future<boost::iterator_range<const uint8_t*>> operator()(const struct video_format_desc& format_desc) = 0;
};

}}