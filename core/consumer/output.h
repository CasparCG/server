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

#include <common/forward.h>
#include <common/future_fwd.h>
#include <common/memory.h>
#include <common/reactive.h>

#include <boost/property_tree/ptree_fwd.hpp>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {
	
class output sealed : public monitor::observable
{
	output(const output&);
	output& operator=(const output&);
public:

	// Static Members

	// Constructors

	explicit output(spl::shared_ptr<diagnostics::graph> graph, const struct video_format_desc& format_desc, int channel_index);
	
	// Methods

	void operator()(class const_frame frame, const struct video_format_desc& format_desc);
	
	void add(const spl::shared_ptr<class frame_consumer>& consumer);
	void add(int index, const spl::shared_ptr<class frame_consumer>& consumer);
	void remove(const spl::shared_ptr<class frame_consumer>& consumer);
	void remove(int index);
	
	// monitor::observable

	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;

	// Properties

	boost::unique_future<boost::property_tree::wptree> info() const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}