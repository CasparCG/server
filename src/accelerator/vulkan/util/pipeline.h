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

#include "uniform_block.h"
#include <vulkan/vulkan.hpp>
namespace caspar { namespace accelerator { namespace vulkan {

enum class shader_flags : uint32_t
{
    none              = 0,
    is_straight_alpha = 1 << 0,
    has_local_key     = 1 << 1,
    has_layer_key     = 1 << 2,
    invert            = 1 << 3,
    levels            = 1 << 4,
    csb               = 1 << 5,
    chroma            = 1 << 6,
    chroma_show_mask  = 1 << 7
};

class pipeline final
{
    pipeline(const pipeline&);
    pipeline& operator=(const pipeline&);

  public:
    pipeline(vk::Device device, vk::Format format);
    ~pipeline();

    void         draw(vk::CommandBuffer                   commandBuffer,
                      vk::Buffer                          vertexBuffer,
                      uint32_t                            coords_count,
                      uint32_t                            vertex_buffer_offset,
                      const uniform_block&                params,
                      const std::array<vk::ImageView, 7>& textures);
    vk::Pipeline id() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
