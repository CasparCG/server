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

#include "frame_producer.h"

#include <common/forward.h>
#include <common/memory/safe_ptr.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

FORWARD1(boost, template<typename T> class unique_future);
FORWARD1(boost, template<typename T> class optional);

namespace caspar { namespace core {
	
class layer sealed
{
public:
	layer(); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow

	void swap(layer& other); // nothrow 
		
	void load(safe_ptr<struct frame_producer> producer, const boost::optional<int32_t>& auto_play_delta = nullptr); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow
		
	safe_ptr<struct frame_producer> foreground() const; // nothrow
	safe_ptr<struct frame_producer> background() const; // nothrow

	safe_ptr<class draw_frame> receive(frame_producer::flags flags); // nothrow

	boost::property_tree::wptree info() const;
private:
	struct impl;
	safe_ptr<impl> impl_;
};

}}