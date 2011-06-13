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

#include <common/memory/safe_ptr.h>

#include <core/producer/frame/frame_visitor.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

class write_frame;
class host_buffer;
class video_channel_context;
struct pixel_format_desc;

class image_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	image_mixer(video_channel_context& context);
	
	virtual void begin(const core::basic_frame& frame);
	virtual void visit(core::write_frame& frame);
	virtual void end();

	void begin_layer();
	void end_layer();

	image_mixer& operator=(image_mixer&& other);
	
	safe_ptr<host_buffer> render();

	safe_ptr<write_frame> create_frame(void* tag, const core::pixel_format_desc& format);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}