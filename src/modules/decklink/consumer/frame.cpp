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

#include <tbb/scalable_allocator.h>

namespace caspar { namespace decklink {

std::shared_ptr<void> convert_to_key_only(const std::shared_ptr<void>& image_data, std::size_t byte_count)
{
    auto key_data = std::shared_ptr<void>(scalable_aligned_malloc(byte_count, 64), scalable_aligned_free);

    aligned_memshfl(key_data.get(), image_data.get(), byte_count, 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);

    return key_data;
}

void convert_frame(const core::video_format_desc& channel_format_desc,
                   const core::video_format_desc& decklink_format_desc,
                   const port_configuration&    config,
                   std::shared_ptr<void>&         image_data,
                   bool                           topField,
                   const core::const_frame&       frame)
{
    // No point copying an empty frame
    if (!frame)
        return;

    int firstLine = topField ? 0 : 1;

    if (channel_format_desc.format == decklink_format_desc.format && config.src_x == 0 && config.src_y == 0 &&
        config.region_w == 0 && config.region_h == 0 && config.dest_x == 0 && config.dest_y == 0) {
        // Fast path
        size_t byte_count_line = (size_t)decklink_format_desc.width * 4;
        for (int y = firstLine; y < decklink_format_desc.height; y += decklink_format_desc.field_count) {
            std::memcpy(reinterpret_cast<char*>(image_data.get()) + (long long)y * byte_count_line,
                        frame.image_data(0).data() + (long long)y * byte_count_line,
                        byte_count_line);
        }
    } else {
        // Take a sub-region

        // Some repetetive numbers
        size_t byte_count_dest_line  = (size_t)decklink_format_desc.width * 4;
        size_t byte_count_src_line   = (size_t)channel_format_desc.width * 4;
        size_t byte_offset_src_line  = std::max(0, (config.src_x * 4));
        size_t byte_offset_dest_line = std::max(0, (config.dest_x * 4));
        int    y_skip_src_lines      = std::max(0, config.src_y);
        int    y_skip_dest_lines     = std::max(0, config.dest_y);

        size_t byte_copy_per_line =
            std::min(byte_count_src_line - byte_offset_src_line, byte_count_dest_line - byte_offset_dest_line);
        if (config.region_w > 0) // If the user chose a width, respect that
            byte_copy_per_line = std::min(byte_copy_per_line, (size_t)config.region_w * 4);

        size_t byte_pad_end_of_line =
            std::max(((size_t)decklink_format_desc.width * 4) - byte_copy_per_line - byte_offset_dest_line, (size_t)0);

        int copy_line_count =
            std::min(channel_format_desc.height - y_skip_src_lines, decklink_format_desc.height - y_skip_dest_lines);
        if (config.region_h > 0) // If the user chose a height, respect that
            copy_line_count = std::min(copy_line_count, config.region_h);

        for (int y = firstLine; y < y_skip_dest_lines; y += decklink_format_desc.field_count) {
            // Fill the line with black
            std::memset(
                reinterpret_cast<char*>(image_data.get()) + (byte_count_dest_line * y), 0, byte_count_dest_line);
        }

        int firstFillLine = y_skip_dest_lines;
        if (decklink_format_desc.field_count != 1 && firstFillLine % 2 != firstLine)
            firstFillLine += 1;
        for (int y = firstFillLine; y < y_skip_dest_lines + copy_line_count; y += decklink_format_desc.field_count) {
            auto line_start_ptr   = reinterpret_cast<char*>(image_data.get()) + (long long)y * byte_count_dest_line;
            auto line_content_ptr = line_start_ptr + byte_offset_dest_line; // Future

            // Fill the start with black
            if (byte_offset_dest_line > 0) {
                std::memset(line_start_ptr, 0, byte_offset_dest_line);
            }

            // Copy the pixels
            std::memcpy(line_content_ptr,
                        frame.image_data(0).data() + (long long)(y + y_skip_src_lines) * byte_count_src_line +
                            byte_offset_src_line,
                        byte_copy_per_line);

            // Fill the end with black
            if (byte_pad_end_of_line > 0) {
                std::memset(line_content_ptr + byte_copy_per_line, 0, byte_pad_end_of_line);
            }
        }

        // Calculate the first line number to fill with black
        int firstPadEndLine = y_skip_dest_lines + copy_line_count;
        if (decklink_format_desc.field_count != 1 && firstPadEndLine % 2 != firstLine)
            firstPadEndLine += 1;
        for (int y = firstPadEndLine; y < decklink_format_desc.height; y += decklink_format_desc.field_count) {
            // Fill the line with black
            std::memset(
                reinterpret_cast<char*>(image_data.get()) + (byte_count_dest_line * y), 0, byte_count_dest_line);
        }
    }
}

std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                         const core::video_format_desc& decklink_format_desc,
                                         const port_configuration&    config,
                                         const core::const_frame&       frame1,
                                         const core::const_frame&       frame2,
                                         BMDFieldDominance              field_dominance)
{
    std::shared_ptr<void> image_data(scalable_aligned_malloc(decklink_format_desc.size, 64), scalable_aligned_free);

    if (field_dominance != bmdProgressiveFrame) {
        convert_frame(channel_format_desc,
                      decklink_format_desc,
                      config,
                      image_data,
                      field_dominance == bmdUpperFieldFirst,
                      frame1);

        convert_frame(channel_format_desc,
                      decklink_format_desc,
                      config,
                      image_data,
                      field_dominance != bmdUpperFieldFirst,
                      frame2);

    } else {
        convert_frame(channel_format_desc, decklink_format_desc, config, image_data, true, frame1);
    }

    if (config.key_only) {
        image_data = convert_to_key_only(image_data, decklink_format_desc.size);
    }

    return image_data;
}

}} // namespace caspar::decklink