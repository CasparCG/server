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

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <functional>

namespace caspar { namespace core {

struct video_format_desc;

class frame_producer_device : boost::noncopyable
{
public:
	typedef std::function<void(const std::map<int, safe_ptr<basic_frame>>&)> output_t;

	explicit frame_producer_device(const video_format_desc& format_desc, const output_t& output);
	frame_producer_device(frame_producer_device&& other);

	void swap(frame_producer_device& other);
		
	void load(int index, const safe_ptr<frame_producer>& producer, bool preview = false);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	void swap_layer(int index, size_t other_index);
	void swap_layer(int index, size_t other_index, frame_producer_device& other);
	boost::unique_future<safe_ptr<frame_producer>> foreground(size_t index);
	boost::unique_future<safe_ptr<frame_producer>> background(size_t index);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}