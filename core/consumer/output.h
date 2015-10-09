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

#include "../monitor/monitor.h"
#include "../fwd.h"

#include <common/forward.h>
#include <common/future_fwd.h>
#include <common/memory.h>
#include <common/reactive.h>

#include <boost/property_tree/ptree_fwd.hpp>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {
	
class output final
{
	output(const output&);
	output& operator=(const output&);
public:

	// Static Members

	// Constructors

	explicit output(spl::shared_ptr<caspar::diagnostics::graph> graph, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout, int channel_index);
	
	// Methods

	void operator()(const_frame frame, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout);
	
	void add(const spl::shared_ptr<frame_consumer>& consumer);
	void add(int index, const spl::shared_ptr<frame_consumer>& consumer);
	void remove(const spl::shared_ptr<frame_consumer>& consumer);
	void remove(int index);
	
	monitor::subject& monitor_output();

	// Properties

	std::future<boost::property_tree::wptree> info() const;
	std::future<boost::property_tree::wptree> delay_info() const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}