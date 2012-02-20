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

#include <common/forward.h>
#include <common/memory.h>

#include <core/frame/frame_visitor.h>

#include <tbb/cache_aligned_allocator.h>

#include <vector>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {
		
typedef std::vector<int32_t, tbb::cache_aligned_allocator<int32_t>> audio_buffer;

class audio_mixer sealed : public frame_visitor
{
	audio_mixer(const audio_mixer&);
	audio_mixer& operator=(const audio_mixer&);
public:

	// Static Members

	// Constructors

	audio_mixer();

	// Methods
	
	audio_buffer operator()(const struct video_format_desc& format_desc);

	// frame_visitor

	virtual void push(const struct frame_transform& transform);
	virtual void visit(const class const_frame& frame);
	virtual void pop();
	
	// Properties

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}