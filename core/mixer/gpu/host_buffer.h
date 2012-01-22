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
#include <common/enum_class.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
			
class host_buffer : boost::noncopyable
{
public:
	struct usage_def
	{
		enum type
		{
			write_only,
			read_only
		};
	};
	typedef enum_class<usage_def> usage;
	
	const void* data() const;
	void* data();
	int size() const;	
	
private:
	friend class ogl_device;
	friend class device_buffer;

	void bind();
	void unbind();

	void map();
	void unmap();

	host_buffer(std::weak_ptr<ogl_device> parent, int size, usage usage);

	struct impl;
	safe_ptr<impl> impl_;
};

}}