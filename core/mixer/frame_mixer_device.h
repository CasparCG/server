/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "fwd.h"

#include "frame_factory.h"

#include "frame/write_frame.h"
#include "frame/pixel_format.h"

#include <common/memory/safe_ptr.h>

#include <functional>

#include "image/image_mixer.h"
#include "audio/audio_mixer.h"

namespace caspar { namespace core {
	
struct video_format;

class frame_mixer_device : public frame_factory
{
public:
	typedef std::function<void(const safe_ptr<const read_frame>&)> output_func;

	frame_mixer_device(const video_format_desc& format_desc, const output_func& output);
	frame_mixer_device(frame_mixer_device&& other); // nothrow
		
	void send(const std::vector<safe_ptr<draw_frame>>& frames); // nothrow
		
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc);		
	safe_ptr<write_frame> create_frame(size_t width, size_t height, pixel_format::type pix_fmt = pixel_format::bgra);			
	safe_ptr<write_frame> create_frame(pixel_format::type pix_fmt = pixel_format::bgra);
	
	const video_format_desc& get_video_format_desc() const; // nothrow

	image_transform& image(int index);
	audio_transform& audio(int index);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}