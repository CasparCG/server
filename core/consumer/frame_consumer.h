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

#include "../format/video_format.h"
#include "../processor/read_frame.h"

#include <boost/noncopyable.hpp>

#include <boost/thread/future.hpp>

#include <memory>

namespace caspar { namespace core {
	
struct frame_consumer : boost::noncopyable
{
	virtual ~frame_consumer() {}

	virtual boost::unique_future<void> prepare(const consumer_frame&)
	{
		boost::promise<void> promise;
		promise.set_value();
		return promise.get_future();
	}

	virtual boost::unique_future<void> display(const consumer_frame&)
	{
		boost::promise<void> promise;
		promise.set_value();
		return promise.get_future();
	}
	virtual bool has_sync_clock() const {return false;}
};
typedef std::shared_ptr<frame_consumer> frame_consumer_ptr;
typedef std::shared_ptr<const frame_consumer> frame_consumer_const_ptr;

typedef std::unique_ptr<frame_consumer> frame_consumer_uptr;
typedef std::unique_ptr<const frame_consumer> frame_consumer_const_uptr;

}}