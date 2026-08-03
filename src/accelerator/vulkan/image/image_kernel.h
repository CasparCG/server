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

#include <core/mixer/image/blend_modes.h>

#include <common/memory.h>
#include <utility>
#include <vulkan/vulkan.hpp>

#include "../util/draw_params.h"
#include "../util/matrix.h"
#include "../util/transforms.h"
#include "../util/uniform_block.h"

namespace caspar { namespace accelerator { namespace vulkan {

class image_kernel final : public std::enable_shared_from_this<image_kernel>
{
    image_kernel(const image_kernel&);
    image_kernel& operator=(const image_kernel&);

  public:
    image_kernel(const spl::shared_ptr<class device>& device, common::bit_depth depth);
    ~image_kernel();

    spl::shared_ptr<class renderpass> create_renderpass(uint32_t width, uint32_t height);

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
