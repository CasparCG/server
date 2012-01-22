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
#include <common/forward.h>

#include <boost/noncopyable.hpp>

FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {
		
class host_buffer;
class ogl_device;

class device_buffer : boost::noncopyable
{
public:	
	int stride() const;	
	int width() const;
	int height() const;
		
	void copy_from(const safe_ptr<host_buffer>& source);
	void copy_to(const safe_ptr<host_buffer>& dest);
private:
	friend class ogl_device;
	friend class image_kernel;
	device_buffer(std::weak_ptr<ogl_device> parent, int width, int height, int stride);
			
	void bind(int index);
	void unbind();
	int id() const;

	struct impl;
	safe_ptr<impl> impl_;
};
	
unsigned int format(int stride);

}}