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

namespace caspar { namespace accelerator { namespace vulkan {

struct uniform_block
{
    uint32_t color_space_index   = 0;
    float    precision_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    int32_t  blend_mode          = 0;
    int32_t  keyer               = 0;
    int32_t  pixel_format        = 0;
    float    opacity             = 1.0;

    /* levels */
    float min_input  = 0;
    float max_input  = 0;
    float gamma      = 0;
    float min_output = 0;
    float max_output = 0;

    /* contrast, saturation & brightness */
    float brt = 0;
    float sat = 0;
    float con = 0;

    /* Chroma */
    float chroma_target_hue                = 0;
    float chroma_hue_width                 = 0;
    float chroma_min_saturation            = 0;
    float chroma_min_brightness            = 0;
    float chroma_softness                  = 0;
    float chroma_spill_suppress            = 0;
    float chroma_spill_suppress_saturation = 0;

    uint32_t flags = 0;
};

}}} // namespace caspar::accelerator::vulkan