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
#include <common/no_copy.h>
#include <common/memory/safe_ptr.h>

#include <core/producer/frame/frame_visitor.h>

FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {
	
class image_mixer sealed : public frame_visitor
{
	CASPAR_NO_COPY(image_mixer);
public:
	image_mixer(const safe_ptr<class ogl_device>& ogl);
	
	virtual void begin(class basic_frame& frame);
	virtual void visit(class write_frame& frame);
	virtual void end();

	void begin_layer(blend_mode blend_mode);
	void end_layer();
		
	boost::unique_future<safe_ptr<class host_buffer>> operator()(const struct video_format_desc& format_desc);
		
private:
	struct impl;
	safe_ptr<impl> impl_;
};

}}