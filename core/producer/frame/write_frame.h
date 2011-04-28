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

#include "basic_frame.h"

#include "../../video_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
class write_frame : public basic_frame, boost::noncopyable
{
public:			
	virtual boost::iterator_range<unsigned char*> image_data(size_t plane_index = 0) = 0;
	virtual std::vector<short>& audio_data() = 0;
	
	virtual const boost::iterator_range<const unsigned char*> image_data(size_t plane_index = 0) const = 0;
	virtual const boost::iterator_range<const short*> audio_data() const = 0;

	virtual void accept(frame_visitor& visitor) = 0;

	virtual int tag() const = 0;
};

}}