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

#include "../write_frame.h"

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>
#include <core/producer/frame/frame_visitor.h>
#include <core/producer/frame/pixel_format.h>

#include "../gpu/host_buffer.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <vector>

namespace caspar { namespace core {
	
class image_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	image_mixer(const core::video_format_desc& format_desc);
	
	virtual void begin(const core::basic_frame& frame);
	virtual void visit(core::write_frame& frame);
	virtual void end();

	void begin_layer();
	void end_layer();
	
	boost::unique_future<safe_ptr<const host_buffer>> render();

	safe_ptr<write_frame> create_frame(void* tag, const core::pixel_format_desc& format);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}