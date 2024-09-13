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
 * Author: Niklas Andersson, niklas@nxtedition.com
 */

#include "../StdAfx.h"

#include "frame.h"
#include "frame_factory_hdr_rgbxle.h"

#include <common/memshfl.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

namespace caspar { namespace decklink {

struct frame_factory_hdr_rgbxle::impl final
{
  public:
    impl() = default;

    BMDPixelFormat get_pixel_format() { return bmdFormat10BitRGBXLE; }

    int get_row_bytes(int width) { return ((width + 63) / 64) * 256; }

    std::shared_ptr<void> allocate_frame_data(const core::video_format_desc& format_desc)
    {
        auto size = get_row_bytes(format_desc.width) * format_desc.height;
        return create_aligned_buffer(format_desc.size, 256);
    }

    void convert_frame(const core::video_format_desc& channel_format_desc,
                       const core::video_format_desc& decklink_format_desc,
                       const port_configuration&      config,
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

            const int NUM_THREADS     = 4;
            auto      rows_per_thread = decklink_format_desc.height / NUM_THREADS;
            size_t    byte_count_line = get_row_bytes(decklink_format_desc.width);
            tbb::parallel_for(0, NUM_THREADS, [&](int i) {
                auto end = (i + 1) * rows_per_thread;
                for (int y = firstLine + i * rows_per_thread; y < end; y += decklink_format_desc.field_count) {
                    auto dest = reinterpret_cast<uint32_t*>(image_data.get()) + (long long)y * byte_count_line / 4;
                    for (int x = 0; x < decklink_format_desc.width; x += 1) {
                        auto src = reinterpret_cast<const uint16_t*>(
                            frame.image_data(0).data() + (long long)y * decklink_format_desc.width * 8 + x * 8);
                        uint16_t blue  = src[0] >> 6;
                        uint16_t green = src[1] >> 6;
                        uint16_t red   = src[2] >> 6;
                        dest[x]        = ((uint32_t)(red) << 22) + ((uint32_t)(green) << 12) + ((uint32_t)(blue) << 2);
                    }
                }
            });
        } else {
            // Take a sub-region
            // TODO: Add support for hdr frames
        }
    }

    std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                                 const core::video_format_desc& decklink_format_desc,
                                                 const port_configuration&      config,
                                                 const core::const_frame&       frame1,
                                                 const core::const_frame&       frame2,
                                                 BMDFieldDominance              field_dominance)
    {
        std::shared_ptr<void> image_data = allocate_frame_data(decklink_format_desc);

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
            // TODO: Add support for hdr frames
        }

        return image_data;
    }
};

frame_factory_hdr_rgbxle::frame_factory_hdr_rgbxle()
    : impl_(new impl())
{
}

frame_factory_hdr_rgbxle::~frame_factory_hdr_rgbxle() {}

BMDPixelFormat frame_factory_hdr_rgbxle::get_pixel_format() { return impl_->get_pixel_format(); }
int            frame_factory_hdr_rgbxle::get_row_bytes(int width) { return impl_->get_row_bytes(width); }

std::shared_ptr<void> frame_factory_hdr_rgbxle::allocate_frame_data(const core::video_format_desc& format_desc)
{
    return impl_->allocate_frame_data(format_desc);
}
std::shared_ptr<void>
frame_factory_hdr_rgbxle::convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                                 const core::video_format_desc& decklink_format_desc,
                                                 const port_configuration&      config,
                                                 const core::const_frame&       frame1,
                                                 const core::const_frame&       frame2,
                                                 BMDFieldDominance              field_dominance)
{
    return impl_->convert_frame_for_port(
        channel_format_desc, decklink_format_desc, config, frame1, frame2, field_dominance);
}

}} // namespace caspar::decklink
