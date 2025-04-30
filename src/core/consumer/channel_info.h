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
 * Author: Julian Waller, julian@superfly.tv
 */

#pragma once

#include "../frame/pixel_format.h"
#include "common/bit_depth.h"

namespace caspar::core {

struct channel_info
{
    channel_info(int channel_index, common::bit_depth depth, color_space color_space)
    : index(channel_index)
    , depth(depth)
    , default_color_space(color_space)
    {}

    int               index;
    common::bit_depth depth;
    color_space       default_color_space;
};

} // namespace caspar::core
