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
#include <common/memory.h>

#include <core/fwd.h>
#include <core/monitor/monitor.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <future>
#include <map>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {

class mixer final
{
	mixer(const mixer&);
	mixer& operator=(const mixer&);
public:

	// Static Members

	// Constructors

	explicit mixer(int channel_index, spl::shared_ptr<caspar::diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer);

	// Methods

	const_frame operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout);

	void set_master_volume(float volume);
	float get_master_volume();
	void set_straight_alpha_output(bool value);
	bool get_straight_alpha_output();

	mutable_frame create_frame(const void* tag, const pixel_format_desc& desc, const core::audio_channel_layout& channel_layout);

	// Properties

	std::future<boost::property_tree::wptree> info() const;
	std::future<boost::property_tree::wptree> delay_info() const;

	monitor::subject& monitor_output();

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}