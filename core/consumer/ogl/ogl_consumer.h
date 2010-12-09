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

#include "../../consumer/frame_consumer.h"

namespace caspar { namespace core { namespace ogl {

struct ogl_error : virtual caspar_exception{};

enum stretch
{
	none,
	uniform,
	fill,
	uniform_to_fill
};

class consumer : public frame_consumer
{
public:	
	explicit consumer(const video_format_desc& format_desc, unsigned int screen_index = 0, stretch stretch = stretch::fill, bool windowed = false);
	
	virtual boost::unique_future<void> display(const consumer_frame& frame);
	virtual bool has_sync_clock() const {return false;}
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}