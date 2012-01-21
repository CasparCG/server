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

#include "image/blend_modes.h"

#include <common/forward.h>
#include <common/memory/safe_ptr.h>
#include <common/concurrency/target.h>

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include <map>

FORWARD2(caspar, diagnostics, class graph);
FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {
	
class mixer sealed : public target<std::pair<std::map<int, safe_ptr<class basic_frame>>, std::shared_ptr<void>>>
				   , boost::noncopyable
{
public:	
	typedef target<std::pair<safe_ptr<class read_frame>, std::shared_ptr<void>>> target_t;

	explicit mixer(const safe_ptr<target_t>& target, const safe_ptr<diagnostics::graph>& graph, const struct video_format_desc& format_desc, const safe_ptr<class ogl_device>& ogl);
		
	// target

	virtual void send(const std::pair<std::map<int, safe_ptr<class basic_frame>>, std::shared_ptr<void>>& frames) override; 
		
	// mixer
		
	void set_video_format_desc(const struct video_format_desc& format_desc);
	
	void set_blend_mode(int index, blend_mode value);

	boost::unique_future<boost::property_tree::wptree> info() const;
	
private:
	struct impl;
	safe_ptr<impl> impl_;
};

}}