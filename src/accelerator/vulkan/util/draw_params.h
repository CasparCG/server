/*
 * Copyright 2025
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
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

#pragma once

#include "transforms.h"
#include <common/memory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <vector>

namespace caspar { namespace accelerator { namespace vulkan {

enum class keyer
{
    linear = 0,
    additive,
};

struct draw_params final
{
    core::pixel_format_desc                     pix_desc = core::pixel_format_desc(core::pixel_format::invalid);
    std::vector<spl::shared_ptr<class texture>> textures;
    draw_transforms                             transforms;
    core::frame_geometry                        geometry   = core::frame_geometry::get_default();
    core::blend_mode                            blend_mode = core::blend_mode::normal;
    vulkan::keyer                               keyer      = vulkan::keyer::linear;
    std::shared_ptr<class texture>              background;
    std::shared_ptr<class texture>              local_key;
    std::shared_ptr<class texture>              layer_key;
    double                                      aspect_ratio = 1.0;
    int                                         target_width;
    int                                         target_height;
};

}}} // namespace caspar::accelerator::vulkan