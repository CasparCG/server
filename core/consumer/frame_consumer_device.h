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

#include "../consumer/frame_consumer.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

namespace caspar { 
	
class executor;
	
namespace core {
	
class video_channel_context;;

class frame_consumer_device : boost::noncopyable
{
public:
	explicit frame_consumer_device(video_channel_context& video_channel);

	void add(int index, safe_ptr<frame_consumer>&& consumer);
	void remove(int index);

	void operator()(const safe_ptr<read_frame>& frame); // nothrow
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}