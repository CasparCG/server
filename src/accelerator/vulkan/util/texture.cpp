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

#include "texture.h"
#include "buffer.h"

#include <common/bit_depth.h>
#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

struct texture::impl
{
    vk::Image         image_;
    vk::DeviceMemory  memory_;
    vk::ImageView     imageView_;
    vk::Device        device_;
    GLsizei           width_  = 0;
    GLsizei           height_ = 0;
    GLsizei           stride_ = 0;
    GLsizei           size_   = 0;
    common::bit_depth depth_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(int               width,
         int               height,
         int               stride,
         common::bit_depth depth,
         vk::Image         image,
         vk::DeviceMemory  memory,
         vk::ImageView     imageView,
         vk::Device        device)
        : image_(image)
        , memory_(memory)
        , imageView_(imageView)
        , device_(device)
        , width_(width)
        , height_(height)
        , stride_(stride)
        , depth_(depth)
        , size_(width * height * stride * (depth == common::bit_depth::bit8 ? 1 : 2))
    {
    }

    ~impl()
    {
        device_.destroyImageView(imageView_);
        device_.freeMemory(memory_);
        device_.destroyImage(image_);
    }
};

texture::texture(int               width,
                 int               height,
                 int               stride,
                 common::bit_depth depth,
                 vk::Image         image,
                 vk::DeviceMemory  memory,
                 vk::ImageView     imageView,
                 vk::Device        device)
    : impl_(new impl(width, height, stride, depth, image, memory, imageView, device))
{
}
texture::texture(texture&& other)
    : impl_(std::move(other.impl_))
{
}
texture::~texture() {}
texture& texture::operator=(texture&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}

vk::ImageView texture::view() const { return impl_->imageView_; }

int               texture::width() const { return impl_->width_; }
int               texture::height() const { return impl_->height_; }
int               texture::stride() const { return impl_->stride_; }
common::bit_depth texture::depth() const { return impl_->depth_; }
void              texture::set_depth(common::bit_depth depth) { impl_->depth_ = depth; }
int               texture::size() const { return impl_->size_; }
VkImage           texture::id() const { return impl_->image_; }

}}} // namespace caspar::accelerator::vulkan
