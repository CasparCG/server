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

#include "../StdAfx.h"

#include "frame.h"

#include <common/memshfl.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

namespace caspar { namespace decklink {

BMDPixelFormat get_pixel_format(const configuration::pixel_format_t& pixel_format)
{
    if (pixel_format == configuration::pixel_format_t::yuv10)
        return bmdFormat10BitYUV;
    // RGB and fallback
    return bmdFormat8BitBGRA;
}
int get_row_bytes(const core::video_format_desc& format_desc, const configuration::pixel_format_t& pixel_format)
{
    if (pixel_format == configuration::pixel_format_t::yuv10)
        return ((format_desc.width + 47) / 48) * 128;
    // RGB and fallback
    return format_desc.width * 4;
}

core::frame_conversion_format get_download_format_for_port(const configuration&           root_config,
                                                           const port_configuration&      port_config,
                                                           const core::video_format_desc& decklink_format_desc)
{
    auto download_pixel_format = core::frame_conversion_format::pixel_format::bgra8;
    switch (root_config.pixel_format) {
        case configuration::pixel_format_t::bgra:
            download_pixel_format = core::frame_conversion_format::pixel_format::bgra8;
            break;
        case configuration::pixel_format_t::yuv10:
            // TODO - choose correct yuv space
            download_pixel_format = core::frame_conversion_format::pixel_format::v210_709;
            break;
    }

    auto download_format =
        core::frame_conversion_format(download_pixel_format, decklink_format_desc.width, decklink_format_desc.height);

    download_format.key_only = port_config.key_only;
    download_format.region   = port_config.region;

    return download_format;
}

std::shared_ptr<void> allocate_frame_data(const core::video_format_desc&       format_desc,
                                          const configuration::pixel_format_t& pixel_format)
{
    auto size = get_row_bytes(format_desc, pixel_format) * format_desc.height;
    return create_aligned_buffer(size, 256);
}

void convert_frame(const core::video_format_desc&   decklink_format_desc,
                   std::shared_ptr<void>&           image_data,
                   bool                             topField,
                   const array<const std::uint8_t>& frame,
                   configuration::pixel_format_t    pixel_format)
{
    // No point copying an empty frame
    if (!frame)
        return;

    int firstLine = topField ? 0 : 1;

    size_t byte_count_line = get_row_bytes(decklink_format_desc, pixel_format);
    for (int y = firstLine; y < decklink_format_desc.height; y += decklink_format_desc.field_count) {
        std::memcpy(reinterpret_cast<char*>(image_data.get()) + (long long)y * byte_count_line,
                    frame.data() + (long long)y * byte_count_line,
                    byte_count_line);
    }
}

std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc&   decklink_format_desc,
                                             const array<const std::uint8_t>& frame1,
                                             const array<const std::uint8_t>& frame2,
                                             BMDFieldDominance                field_dominance,
                                             configuration::pixel_format_t    pixel_format)
{
    std::shared_ptr<void> image_data = allocate_frame_data(decklink_format_desc, pixel_format);

    // These copies feel a bit wasteful, but we need to correct the buffer alignment

    if (field_dominance != bmdProgressiveFrame) {
        convert_frame(decklink_format_desc, image_data, field_dominance == bmdUpperFieldFirst, frame1, pixel_format);

        convert_frame(decklink_format_desc, image_data, field_dominance != bmdUpperFieldFirst, frame2, pixel_format);

    } else {
        convert_frame(decklink_format_desc, image_data, true, frame1, pixel_format);
    }

    return image_data;
}

}} // namespace caspar::decklink
