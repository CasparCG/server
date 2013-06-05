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

#include "image/blend_modes.h"

#include "../producer/frame/frame_factory.h"

#include <common/memory/safe_ptr.h>
#include <common/concurrency/target.h>
#include <common/diagnostics/graph.h>

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/thread/future.hpp>

#include <map>

namespace caspar { 

class executor;
	
namespace core {

class read_frame;
class write_frame;
class basic_frame;
class ogl_device;
struct frame_transform;
struct pixel_format;
struct channel_layout;

class mixer : public target<std::pair<std::map<int, safe_ptr<core::basic_frame>>, std::shared_ptr<void>>>
			, public core::frame_factory
{
public:	
	typedef target<std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>> target_t;

	explicit mixer(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl, const channel_layout& audio_channel_layout);
		
	// target

	virtual void send(const std::pair<std::map<int, safe_ptr<basic_frame>>, std::shared_ptr<void>>& frames) override; 
		
	// mixer

	safe_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc, const channel_layout& audio_channel_layout);		
	
	core::video_format_desc get_video_format_desc() const; // nothrow
	void set_video_format_desc(const video_format_desc& format_desc);
	
	blend_mode::type get_blend_mode(int index);
	void set_blend_mode(int index, blend_mode::type value);
	chroma get_chroma(int index);
    void set_chroma(int index, const chroma& value);
	void clear_blend_mode(int index);
	void clear_blend_modes();

	float get_master_volume();
	void set_master_volume(float volume);

	boost::unique_future<boost::property_tree::wptree> info() const;
	
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}