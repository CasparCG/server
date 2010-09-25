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
 
#include "../stdafx.h"

#include "system_frame.h"
#include "format.h"

#include <tbb/scalable_allocator.h>

namespace caspar {

system_frame::system_frame(const frame_format_desc& format_desc) 
	: size_(format_desc.size), width_(format_desc.width), height_(format_desc.height), data_(static_cast<unsigned char*>(scalable_aligned_malloc(format_desc.size, 16))){}
system_frame::system_frame(size_t width, size_t height) 
	: size_(width*height*4), width_(width), height_(height), data_(static_cast<unsigned char*>(scalable_aligned_malloc(width*height*4, 16))){}
system_frame::system_frame(const frame_format_desc& format_desc, size_t size) 
	: size_(format_desc.size), width_(format_desc.width), height_(format_desc.height), data_(static_cast<unsigned char*>(scalable_aligned_malloc(size, 16))){}
system_frame::system_frame(size_t width, size_t height, size_t size) 
	: size_(width*height*4), width_(width), height_(height), data_(static_cast<unsigned char*>(scalable_aligned_malloc(size, 16))){}
system_frame::~system_frame(){scalable_aligned_free(data_);}
unsigned char* system_frame::data() {return data_;}	
size_t system_frame::size() const{return size_;}
size_t system_frame::width() const {return width_;}
size_t system_frame::height() const {return height_;}

}

