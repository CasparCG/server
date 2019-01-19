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
* Author: Robert Nagy, ronag89@gmail.com
          James Wise, james.wise@bluefish444.com
*/

#pragma once

#include <Windows.h>
#include <boost/align.hpp>
#include <vector>

namespace caspar { namespace bluefish {

static const size_t MAX_HANC_BUFFER_SIZE = 256 * 1024;
static const size_t MAX_VBI_BUFFER_SIZE  = 36 * 1920 * 4;

struct blue_dma_buffer
{
  public:
    blue_dma_buffer(int image_size, int id)
        : id_(id)
        , image_size_(image_size)
        , hanc_size_(MAX_HANC_BUFFER_SIZE)
        , image_buffer_(image_size_)
        , hanc_buffer_(hanc_size_)
    {
    }

    int id() const { return id_; }

    PBYTE image_data() { return image_buffer_.data(); }
    PBYTE hanc_data() { return hanc_buffer_.data(); }

    size_t image_size() const { return image_size_; }
    size_t hanc_size() const { return hanc_size_; }

  private:
    int                                                              id_;
    size_t                                                           image_size_;
    size_t                                                           hanc_size_;
    std::vector<BYTE, boost::alignment::aligned_allocator<BYTE, 64>> image_buffer_;
    std::vector<BYTE, boost::alignment::aligned_allocator<BYTE, 64>> hanc_buffer_;
};
using blue_dma_buffer_ptr = std::shared_ptr<blue_dma_buffer>;

}} // namespace caspar::bluefish
