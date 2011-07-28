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
#include "image.h"

#include "producer/image_producer.h"
#include "producer/image_scroll_producer.h"

#include <core/producer/frame_producer.h>

#include <common/utility/string.h>

#include <FreeImage.h>

namespace caspar {

void init_image()
{
	core::register_producer_factory(create_image_scroll_producer);
	core::register_producer_factory(create_image_producer);
}

std::wstring get_image_version()
{
	return widen(std::string(FreeImage_GetVersion()));
}

}