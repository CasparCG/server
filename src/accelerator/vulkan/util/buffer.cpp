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

#include "buffer.h"

#include <boost/property_tree/ptree.hpp>

#include <vulkan/vulkan.hpp>

#pragma warning(push)
#pragma warning(disable : 4189)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include <atomic>

namespace caspar { namespace accelerator { namespace vulkan {

static std::atomic<int>         g_w_total_count;
static std::atomic<std::size_t> g_w_total_size;
static std::atomic<int>         g_r_total_count;
static std::atomic<std::size_t> g_r_total_size;

struct buffer::impl
{
    int  size_  = 0;
    bool write_ = false;

    VkBuffer          buf;
    VmaAllocation     alloc;
    VmaAllocationInfo allocInfo;

    VmaAllocator allocator_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(int size, bool write, VmaAllocator allocator)
        : size_(size)
        , write_(write)
        , allocator_(allocator)
    {
        VkBufferCreateInfo bufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufCreateInfo.size               = size;
        bufCreateInfo.usage              = write ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags =
            write ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
                  : VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(allocator_, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, &allocInfo);

        (write ? g_w_total_count : g_r_total_count)++;
        (write ? g_w_total_size : g_r_total_size) += size_;
    }

    ~impl()
    {
        vmaDestroyBuffer(allocator_, buf, alloc);

        (write_ ? g_w_total_size : g_r_total_size) -= size_;
        (write_ ? g_w_total_count : g_r_total_count)--;
    }
};

buffer::buffer(int size, bool write, VmaAllocator allocator)
    : impl_(new impl(size, write, allocator))
{
}
buffer::buffer(buffer&& other)
    : impl_(std::move(other.impl_))
{
}
buffer::~buffer() {}
buffer& buffer::operator=(buffer&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}
void*    buffer::data() { return impl_->allocInfo.pMappedData; }
bool     buffer::write() const { return impl_->write_; }
int      buffer::size() const { return static_cast<int>(impl_->allocInfo.size); }
VkBuffer buffer::id() const { return impl_->buf; }

boost::property_tree::wptree buffer::info()
{
    boost::property_tree::wptree info;

    info.add(L"total_read_count", g_r_total_count);
    info.add(L"total_write_count", g_w_total_count);
    info.add(L"total_read_size", g_r_total_size);
    info.add(L"total_write_size", g_w_total_size);

    return info;
}

}}} // namespace caspar::accelerator::vulkan
