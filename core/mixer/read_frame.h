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

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/thread/future.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
class host_buffer;
class ogl_device;

class read_frame : boost::noncopyable
{
	read_frame(){}
public:
	read_frame(safe_ptr<host_buffer>&& image_data, std::vector<int16_t>&& audio_data);

	virtual const boost::iterator_range<const uint8_t*> image_data() const;
	virtual const boost::iterator_range<const int16_t*> audio_data() const;
		
	static safe_ptr<const read_frame> empty()
	{
		struct empty : public read_frame
		{			
			virtual const boost::iterator_range<const uint8_t*> image_data() const {return boost::iterator_range<const uint8_t*>();}
			virtual const boost::iterator_range<const int16_t*> audio_data() const {return boost::iterator_range<const int16_t*>();}
		};
		static safe_ptr<const empty> frame;
		return frame;
	}
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}