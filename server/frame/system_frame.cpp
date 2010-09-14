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

#include <tbb/scalable_allocator.h>

namespace caspar {

system_frame::system_frame(unsigned int dataSize) 
	: size_(dataSize), data_(static_cast<unsigned char*>(scalable_aligned_malloc(dataSize, 16))){}
system_frame::~system_frame(){scalable_aligned_free(data_);}
unsigned char* system_frame::data() {return data_;}	
unsigned int system_frame::size() const{return size_;}

}

