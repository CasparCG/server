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

#include <boost/property_tree/ptree_fwd.hpp>
#include <memory>

namespace caspar::accelerator::ogl {

// This must match description_layout in shader_from_rgba.comp
struct convert_from_texture_description
{
    uint32_t target_format;
    uint32_t is_16_bit;
    uint32_t width;
    uint32_t height;
    uint32_t words_per_line;
    uint32_t key_only;
    uint32_t straighten;
};

} // namespace caspar::accelerator::ogl
