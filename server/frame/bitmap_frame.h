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

#include "frame.h"

#include <memory>

namespace caspar{

class bitmap_frame : public frame
{
public:
	bitmap_frame(size_t width, size_t height);
	bitmap_frame(const std::shared_ptr<frame>& frame);

	unsigned char* data();
	size_t size() const;	
	size_t width() const;
	size_t height() const;
	HDC hdc();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<bitmap_frame> bitmap_frame_ptr;
typedef std::unique_ptr<bitmap_frame> bitmap_frame_uptr;

}

