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
#include "../processor/frame.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {
	
struct frame_consumer : boost::noncopyable
{
	virtual ~frame_consumer() {}

	virtual void prepare(const frame_ptr&){}
	virtual void display(const frame_ptr&){}
	virtual bool has_sync_clock() const {return false;}
};
typedef std::shared_ptr<frame_consumer> frame_consumer_ptr;
typedef std::shared_ptr<const frame_consumer> frame_consumer_const_ptr;

typedef std::unique_ptr<frame_consumer> frame_consumer_uptr;
typedef std::unique_ptr<const frame_consumer> frame_consumer_const_uptr;

}}