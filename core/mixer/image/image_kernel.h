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

#include "blend_modes.h"

#include <common/no_copy.h>
#include <common/enum_class.h>
#include <common/memory/safe_ptr.h>

#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/frame_transform.h>

namespace caspar { namespace core {
	
CASPAR_BEGIN_ENUM_CLASS
{
	linear = 0,
	additive
}
CASPAR_END_ENUM_CLASS(keyer)

struct draw_params sealed
{
	pixel_format_desc							pix_desc;
	std::vector<safe_ptr<class device_buffer>>	textures;
	frame_transform								transform;
	blend_mode									blend_mode;
	keyer										keyer;
	std::shared_ptr<class device_buffer>		background;
	std::shared_ptr<class device_buffer>		local_key;
	std::shared_ptr<class device_buffer>		layer_key;

	draw_params() 
		: blend_mode(blend_mode::normal)
		, keyer(keyer::linear)
	{
	}
};

class image_kernel sealed
{
	CASPAR_NO_COPY(image_kernel);
public:
	image_kernel(const safe_ptr<class ogl_device>& ogl);
	void draw(draw_params&& params);
private:
	struct impl;
	safe_ptr<impl> impl_;
};

}}