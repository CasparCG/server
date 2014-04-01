/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../monitor/monitor.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

namespace caspar { namespace core {

struct frame_producer;
class basic_frame;

class layer : boost::noncopyable
{
	layer(const layer&);
	layer& operator=(const layer&);
public:
	layer(int index = -1); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow

	void swap(layer& other); // nothrow 
		
	void load(const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void resume(); // nothrow
	void stop(); // nothrow
	boost::unique_future<std::wstring> call(bool foreground, const std::wstring& param);

	bool is_paused() const;
	int64_t frame_number() const;
	
	bool empty() const;

	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<basic_frame> receive(int hints); // nothrow

	boost::property_tree::wptree info() const;
	boost::property_tree::wptree delay_info() const;
	
	monitor::subject& monitor_output();
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}