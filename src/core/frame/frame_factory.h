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
 */

#pragma once

#include <common/bit_depth.h>

#include <future>

namespace caspar { namespace core {

enum encoded_frame_format
{
    decklink_v210 = 0,
};

class frame_converter
{
  public:
    frame_converter()                                  = default;
    frame_converter& operator=(const frame_converter&) = delete;
    virtual ~frame_converter()                         = default;

    frame_converter(const frame_converter&) = delete;

    virtual class mutable_frame create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;

    virtual class draw_frame convert_to_rgba(const class mutable_frame& frame) = 0;

    virtual std::shared_future<array<const std::uint8_t>> convert_from_rgba(const core::const_frame& frame,
                                                                            encoded_frame_format     format) = 0;
};

class frame_factory
{
  public:
    frame_factory()                                = default;
    frame_factory& operator=(const frame_factory&) = delete;
    virtual ~frame_factory()                       = default;

    frame_factory(const frame_factory&) = delete;

    virtual class mutable_frame create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;

    virtual spl::shared_ptr<frame_converter> create_frame_converter() = 0;
};

}} // namespace caspar::core
