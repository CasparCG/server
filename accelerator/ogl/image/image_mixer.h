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

#include <common/forward.h>
#include <common/memory.h>
#include <common/future_fwd.h>

#include <core/mixer/image/blend_modes.h>
#include <core/mixer/image/image_mixer.h>

#include <core/frame/frame_visitor.h>
#include <core/video_format.h>

FORWARD2(caspar, core, class frame);
FORWARD2(caspar, core, struct pixel_format_desc);
FORWARD2(caspar, core, struct video_format_desc);
FORWARD2(caspar, core, class mutable_frame);
FORWARD2(caspar, core, struct frame_transform);

namespace caspar { namespace accelerator { namespace ogl {
	
class image_mixer sealed : public core::image_mixer
{
	image_mixer(const image_mixer&);
	image_mixer& operator=(const image_mixer&);
public:

	// Static Members
	
	// Constructors

	image_mixer(const spl::shared_ptr<class device>& ogl);
	~image_mixer();

	// Methods
			
	boost::unique_future<array<const std::uint8_t>> operator()(const core::video_format_desc& format_desc) override;		
	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override;

	// core::image_mixer
	
	void begin_layer(core::blend_mode blend_mode) override;
	void end_layer() override;

	void push(const core::frame_transform& frame) override;
	void visit(const core::const_frame& frame) override;
	void pop() override;
			
	// Properties

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}