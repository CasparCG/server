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
#include "frame_fwd.h"

#include <memory>

namespace caspar {

class system_frame : public frame
{
public:
	explicit system_frame(const frame_format_desc& format);
	system_frame(size_t width, size_t height);
	system_frame(const frame_format_desc& format, size_t size);
	system_frame(size_t width, size_t height, size_t size);
	~system_frame();

	unsigned char* data();
	size_t size() const;
	size_t width() const;
	size_t height() const;
private:
	unsigned char* const data_;
	const size_t size_;
	const size_t width_;
	const size_t height_;
};

}