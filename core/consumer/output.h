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
#include <common/concurrency/target.h>
#include <common/concurrency/governor.h>
#include <common/diagnostics/graph.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
	
class output : public target<std::pair<safe_ptr<read_frame>, ticket>>, boost::noncopyable
{
public:
	explicit output(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, int channel_index);

	void add(int index, safe_ptr<frame_consumer>&& consumer);
	void remove(int index);
	
	void set_video_format_desc(const video_format_desc& format_desc);

	virtual void send(const std::pair<safe_ptr<read_frame>, ticket>& frame); // nothrow
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}