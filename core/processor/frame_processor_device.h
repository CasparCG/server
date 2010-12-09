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

#include <memory>
#include <vector>

#include "fwd.h"

#include "../format/video_format.h"
#include "../consumer/frame_consumer_device.h"

namespace caspar { namespace core {

class frame_processor_device : boost::noncopyable
{
public:
	frame_processor_device(const video_format_desc& format_desc);
		
	void send(const gpu_frame_ptr& frame);
	consumer_frame receive();
	
	write_frame_ptr create_frame(const pixel_format_desc& desc);		
	write_frame_ptr create_frame(size_t width, size_t height);			
	write_frame_ptr create_frame();
	
	const video_format_desc& get_video_format_desc() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_processor_device> frame_processor_device_ptr;

}}