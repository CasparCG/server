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

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace caspar { namespace core {
	
class host_buffer;
class ogl_device;

class read_frame : boost::noncopyable
{
public:
	read_frame();
	read_frame(const safe_ptr<ogl_device>& ogl, size_t size, safe_ptr<host_buffer>&& image_data, audio_buffer&& audio_data);

	virtual const boost::iterator_range<const uint8_t*> image_data();
	virtual const boost::iterator_range<const int32_t*> audio_data();

	virtual size_t image_size() const;
		
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}