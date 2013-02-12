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

#include <core/producer/frame/frame_visitor.h>

#include <boost/noncopyable.hpp>

#include <tbb/cache_aligned_allocator.h>

#include <vector>

namespace caspar {
	
namespace diagnostics {
	
class graph;

}

namespace core {

struct video_format_desc;
	
typedef std::vector<int32_t, tbb::cache_aligned_allocator<int32_t>> audio_buffer;

class audio_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	audio_mixer(const safe_ptr<diagnostics::graph>& graph);

	virtual void begin(core::basic_frame& frame);
	virtual void visit(core::write_frame& frame);
	virtual void end();

	void set_master_volume(float volume);

	audio_buffer operator()(const video_format_desc& format_desc);
	
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}