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

#include <common/memory/safe_ptr.h>
#include <common/reactive.h>
#include <common/forward.h>

#include <boost/property_tree/ptree_fwd.hpp>

FORWARD3(caspar, core, gpu, class accelerator);

namespace caspar { namespace core {
	
class video_channel sealed : public reactive::observable<safe_ptr<const struct data_frame>>
{
	video_channel(const video_channel&);
	video_channel& operator=(const video_channel&);
public:
	explicit video_channel(int index, const struct video_format_desc& format_desc, const safe_ptr<gpu::accelerator>& ogl);

	safe_ptr<class stage>			stage();
	safe_ptr<class mixer>			mixer();
	safe_ptr<class output>			output();
	safe_ptr<struct frame_factory>	frame_factory();
	
	struct video_format_desc get_video_format_desc() const;
	void set_video_format_desc(const struct video_format_desc& format_desc);
	
	boost::property_tree::wptree info() const;

	// observable
	
	virtual void subscribe(const observer_ptr& o) override;
	virtual void unsubscribe(const observer_ptr& o) override;
private:
	struct impl;
	safe_ptr<impl> impl_;
};

}}