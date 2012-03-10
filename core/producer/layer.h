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

#include "../monitor/monitor.h"

#include <common/forward.h>
#include <common/future_fwd.h>
#include <common/memory.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

FORWARD1(boost, template<typename T> class optional);

namespace caspar { namespace core {
	
class layer sealed : public monitor::observable
{
	layer(const layer&);
	layer& operator=(const layer&);
public:
	// Static Members

	// Constructors

	explicit layer(int index = -1); 
	layer(layer&& other); 

	// Methods

	layer& operator=(layer&& other); 

	void swap(layer& other);  
		
	void load(spl::shared_ptr<class frame_producer> producer, bool preview, const boost::optional<int32_t>& auto_play_delta = nullptr); 
	void play(); 
	void pause(); 
	void stop(); 
	
	class draw_frame receive(const struct video_format_desc& format_desc); 
	
	// monitor::observable

	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;

	// Properties
		
	spl::shared_ptr<class frame_producer>	foreground() const; 
	spl::shared_ptr<class frame_producer>	background() const; 

	boost::property_tree::wptree			info() const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}