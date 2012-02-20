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

#include <common/memory.h>

#include <cstddef>

namespace caspar { namespace accelerator { namespace ogl {
		
class buffer;
class device;

class texture sealed
{
	texture(const texture&);
	texture& operator=(const texture&);
public:	

	// Static Members

	// Constructors

	texture(int width, int height, int stride);
	texture(texture&& other);
	~texture();
	
	// Methods

	texture& operator=(texture&& other);
		
	void copy_from(buffer& source);
	void copy_to(buffer& dest);
			
	void attach();
	void clear();
	void bind(int index);
	void unbind();

	// Properties

	int width() const;
	int height() const;
	int stride() const;	
	std::size_t size() const;

	int id() const;

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};
	
}}}