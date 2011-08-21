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

#include "blend_modes.h"

#include <common/memory/safe_ptr.h>

#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/frame_transform.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
	
class device_buffer;
class ogl_device;

struct render_item
{
	pixel_format_desc						pix_desc;
	std::vector<safe_ptr<device_buffer>>	textures;
	frame_transform							transform;
	blend_mode::type						blend_mode;
};

bool operator==(const render_item& lhs, const render_item& rhs);

class image_kernel : boost::noncopyable
{
public:
	image_kernel();
	void draw(ogl_device& ogl, 
			  render_item&& item, 
			  device_buffer& background, 
			  const std::shared_ptr<device_buffer>& local_key = nullptr, 
			  const std::shared_ptr<device_buffer>& layer_key = nullptr);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}