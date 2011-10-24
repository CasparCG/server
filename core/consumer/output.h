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

#include <common/concurrency/governor.h>
#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <agents.h>

namespace caspar { namespace core {
	
class video_channel_context;

class output : boost::noncopyable
{
public:
	typedef safe_ptr<std::pair<safe_ptr<core::read_frame>, ticket_t>>	source_element_t;
	typedef Concurrency::ISource<source_element_t>						source_t;

	explicit output(source_t& source, const video_format_desc& format_desc);

	void add(int index, safe_ptr<frame_consumer>&& consumer);
	void remove(int index);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}