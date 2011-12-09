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

#include <boost/noncopyable.hpp>

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {

class ogl_device;
		
class host_buffer : boost::noncopyable
{
public:
	enum usage_t
	{
		write_only,
		read_only
	};
	
	const void* data() const;
	void* data();
	int size() const;	
	
	void bind();
	void unbind();

	void map();
	void unmap();
	
	void begin_read(int width, int height, unsigned int format);
	bool ready() const;
	void wait(ogl_device& ogl);
private:
	friend class ogl_device;
	host_buffer(int size, usage_t usage);

	struct implementation;
	safe_ptr<implementation> impl_;
};

}}