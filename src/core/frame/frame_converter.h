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

#include <common/bit_depth.h>

#include <core/frame/frame.h>

#include <future>

namespace caspar::core {

struct frame_conversion_format
{
    // Note: when adding fields to this, the cache_key calculation must be updated

    enum pixel_format
    {
        bgra8 = 0,
        //        rgba16   = 1,
        //        bgra16   = 2,
        v210_709 = 3,
        v210_601 = 4,
    };

    struct subregion_geometry
    {
        int src_x  = 0;
        int src_y  = 0;
        int dest_x = 0;
        int dest_y = 0;
        int w      = 0;
        int h      = 0;

        [[nodiscard]] bool has_subregion_geometry() const
        {
            return src_x != 0 || src_y != 0 || w != 0 || h != 0 || dest_x != 0 || dest_y != 0;
        }
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

    const pixel_format format;
    const int          width;
    const int          height;

    bool key_only       = false;
    bool straight_alpha = false;

    subregion_geometry region;
};

struct converted_frame
{
    frame_conversion_format::pixel_format         format = frame_conversion_format::pixel_format::bgra8;
    const_frame                                   frame;
    std::shared_future<array<const std::uint8_t>> pixels;

    explicit converted_frame()
    {
    }

    converted_frame(const frame_conversion_format& format, const core::const_frame& frame, std::shared_future<array<const std::uint8_t>> pixels)
        : format(format.format)
        , frame(frame)
        , pixels(std::move(pixels))
    {
    }

    bool is_empty() {
        return frame == core::const_frame{} || !pixels.valid();
    }
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

    virtual std::shared_future<array<const std::uint8_t>> convert_to_buffer(const core::const_frame&       frame,
                                                                            const frame_conversion_format& format) = 0;

    converted_frame convert_to_buffer_and_frame(const core::const_frame& frame, const frame_conversion_format& format)
    {
        auto pixels = this->convert_to_buffer(frame, format);
        return converted_frame(format, frame, pixels);
    }

    virtual common::bit_depth get_frame_bitdepth(const core::const_frame& frame) = 0;
};

} // namespace caspar::core
