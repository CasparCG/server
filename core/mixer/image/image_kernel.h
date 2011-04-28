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

#include <memory>

#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/image_transform.h>

namespace caspar { namespace mixer {
	
class image_kernel
{
public:
	image_kernel();
	void apply(const core::pixel_format_desc& pix_desc, const core::image_transform& mode);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}