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

#include <core/mixer/image/blend_modes.h>

#include <common/enum_class.h>
#include <common/spl/memory.h>

#include <core/frame/pixel_format.h>
#include <core/frame/frame_transform.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace accelerator { namespace ogl {
	
struct keyer_def
{
	enum type
	{
		linear = 0,
		additive,
	};
};
typedef enum_class<keyer_def> keyer;

struct draw_params sealed
{
	core::pixel_format_desc								pix_desc;
	std::vector<spl::shared_ptr<class device_buffer>>	textures;
	core::frame_transform								transform;
	core::blend_mode									blend_mode;
	keyer												keyer;
	std::shared_ptr<class device_buffer>				background;
	std::shared_ptr<class device_buffer>				local_key;
	std::shared_ptr<class device_buffer>				layer_key;

	draw_params() 
		: pix_desc(core::pixel_format::invalid)
		, blend_mode(core::blend_mode::normal)
		, keyer(keyer::linear)
	{
	}
};

class image_kernel sealed : boost::noncopyable
{
public:
	image_kernel(const spl::shared_ptr<class context>& ogl);
	void draw(draw_params&& params);
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}