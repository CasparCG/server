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

#include <common/spl/memory.h>
#include <common/reactive.h>
#include <common/forward.h>

#include "monitor/monitor.h"

#include <boost/property_tree/ptree_fwd.hpp>

FORWARD3(caspar, core, ogl, class accelerator);
FORWARD2(caspar, core, class stage);
FORWARD2(caspar, core, class mixer);
FORWARD2(caspar, core, class output);
FORWARD2(caspar, core, struct image_mixer);
FORWARD2(caspar, core, struct video_format_desc);
FORWARD2(caspar, core, struct frame_factory);

namespace caspar { namespace core {
	
typedef reactive::observable<spl::shared_ptr<const struct data_frame>>	frame_observable;

class video_channel sealed : public frame_observable
						   , public monitor::observable
{
	video_channel(const video_channel&);
	video_channel& operator=(const video_channel&);
public:
	explicit video_channel(int index, const video_format_desc& format_desc, spl::unique_ptr<image_mixer> image_mixer);
	
	const core::stage&	stage() const;
	core::stage&		stage();
	const core::mixer&	mixer() const;
	core::mixer&		mixer();
	const core::output&	output() const;
	core::output&		output();
		
	core::video_format_desc video_format_desc() const;
	void video_format_desc(const core::video_format_desc& format_desc);
	
	spl::shared_ptr<core::frame_factory> frame_factory();

	boost::property_tree::wptree info() const;

	// observable<spl::shared_ptr<const struct data_frame>>
	
	virtual void subscribe(const frame_observable::observer_ptr& o) override;
	virtual void unsubscribe(const frame_observable::observer_ptr& o) override;
		
	// monitor::observable

	virtual void subscribe(const monitor::observable::observer_ptr& o) override;
	virtual void unsubscribe(const monitor::observable::observer_ptr& o) override;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}