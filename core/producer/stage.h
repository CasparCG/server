/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include "frame_producer.h"

#include <common/memory/safe_ptr.h>
#include <common/concurrency/target.h>
#include <common/concurrency/governor.h>
#include <common/diagnostics/graph.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

struct video_format_desc;
struct layer_status;

class stage : boost::noncopyable
{
public:
	typedef target<std::pair<std::map<int, safe_ptr<basic_frame>>, ticket>> target_t;

	explicit stage(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc);

	void swap(stage& other);
			
	void load(int index, const safe_ptr<frame_producer>& producer, bool preview = false, int auto_play_delta = -1);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	void swap_layer(int index, size_t other_index);
	void swap_layer(int index, size_t other_index, stage& other);

	layer_status get_status(int index);
	safe_ptr<frame_producer> foreground(size_t index);
	safe_ptr<frame_producer> background(size_t index);
	
	void set_video_format_desc(const video_format_desc& format_desc);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}