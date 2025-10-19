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

#include <common/bit_depth.h>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class texture final
{
  public:
    texture(int               width,
            int               height,
            int               stride,
            common::bit_depth depth,
            vk::Image         image,
            vk::DeviceMemory  memory,
            vk::ImageView     imageView,
            vk::Device        device);
    texture(const texture&) = delete;
    texture(texture&& other);
    ~texture();

    texture& operator=(const texture&) = delete;
    texture& operator=(texture&& other);

    vk::ImageView view() const;

    int               width() const;
    int               height() const;
    int               stride() const;
    common::bit_depth depth() const;
    void              set_depth(common::bit_depth depth);
    int               size() const;
    VkImage           id() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::vulkan
