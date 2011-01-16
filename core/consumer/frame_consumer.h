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

namespace caspar { namespace core {
	
class read_frame;

struct frame_consumer : boost::noncopyable
{
	virtual ~frame_consumer() {}
	
	virtual void send(const safe_ptr<const read_frame>& frame) = 0;
	virtual size_t buffer_depth() const = 0;

	static safe_ptr<frame_consumer> empty()
	{
		struct empty_frame_consumer : public frame_consumer
		{
			virtual void send(const safe_ptr<const read_frame>&){}
			virtual size_t buffer_depth() const{return 0;}
		};
		static safe_ptr<frame_consumer> consumer = make_safe<empty_frame_consumer>();
		return consumer;
	}
};

}}