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

#include "core/video_format.h"

#include <common/bit_depth.h>

#include <core/frame/frame.h>

#include <future>

namespace caspar::core {

struct frame_conversion_format
{
    enum pixel_format
    {
        bgra8 = 0,
        // rgba16        = 0,
        // bgra16        = 1,
        // decklink_v210 = 2,
    };

    struct subregion_geometry
    {
        int src_x  = 0;
        int src_y  = 0;
        int dest_x = 0;
        int dest_y = 0;
        int w      = 0;
        int h      = 0;
    };

    explicit frame_conversion_format(pixel_format format, int width, int height)
    : format(format)
    , width(width)
    , height(height)
    {
    }

    explicit frame_conversion_format(pixel_format format)
   : frame_conversion_format(format, 0, 0)
    {
    }

    pixel_format format;
    int width;
    int height;

    bool key_only = false;
    bool straight_alpha = false;

    subregion_geometry region;
};


class frame_converter
{
  public:
    frame_converter()                                  = default;
    frame_converter& operator=(const frame_converter&) = delete;
    virtual ~frame_converter()                         = default;

    frame_converter(const frame_converter&) = delete;

    // virtual class mutable_frame create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;

    // virtual class draw_frame convert_to_rgba(const class mutable_frame& frame) = 0;

    virtual std::shared_future<array<const std::uint8_t>>
    convert_to_buffer(const core::const_frame& frame, const frame_conversion_format& format) = 0;

    converted_frame
    convert_to_buffer_and_frame(const core::const_frame& frame, const frame_conversion_format& format)
    {
        auto pixels = this->convert_to_buffer(frame, format);
        return converted_frame(frame, pixels);
    }

    virtual common::bit_depth get_frame_bitdepth(const core::const_frame& frame) = 0;
};

} // namespace caspar::core
