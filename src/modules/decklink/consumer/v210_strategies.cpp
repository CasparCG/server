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

#if !defined(WIN32) && (defined(__x86_64__) || defined(__i386__))
// Force this file to compile with avx2, as it has been crafted with intrinsics that require it.
#pragma GCC target("avx2")
#endif

#ifdef USE_SIMDE
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/avx2.h>
#endif

#include <type_traits>

#include "format_strategy.h"

#include <common/memshfl.h>
#include <common/v210.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

namespace caspar { namespace decklink {

class v210_strategy
    : public format_strategy
    , std::enable_shared_from_this<v210_strategy>
{
    spl::shared_ptr<v210::v210_output> output_;

  public:
    explicit v210_strategy(core::color_space color_space, uint8_t bpc)
        : output_(v210::create_v210_output(color_space, bpc))
    {
    }

    BMDPixelFormat get_pixel_format() override { return bmdFormat10BitYUV; }

    int get_row_bytes(int width) override { return ((width + 47) / 48) * 128; }

    std::shared_ptr<void> allocate_frame_data(const core::video_format_desc& format_desc) override
    {
        auto size = get_row_bytes(format_desc.width) * format_desc.height;
        return create_aligned_buffer(size, 128);
    }
    std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                                 const core::video_format_desc& decklink_format_desc,
                                                 const port_configuration&      config,
                                                 const core::const_frame&       frame1,
                                                 const core::const_frame&       frame2,
                                                 BMDFieldDominance              field_dominance) override
    {
        std::shared_ptr<void> image_data = allocate_frame_data(decklink_format_desc);

        auto region     = v210::output_region{};
        region.src_x    = config.src_x;
        region.src_y    = config.src_y;
        region.dest_x   = config.dest_x;
        region.dest_y   = config.dest_y;
        region.region_w = config.region_w;
        region.region_h = config.region_h;

        output_->convert_frame(channel_format_desc,
                               decklink_format_desc,
                               region,
                               image_data,
                               frame1,
                               frame2,
                               field_dominance != bmdProgressiveFrame  ? 0
                               : field_dominance == bmdUpperFieldFirst ? 1
                                                                       : 2);

        return image_data;
    }
};

spl::shared_ptr<format_strategy> create_sdr_v210_strategy(core::color_space color_space)
{
    return spl::make_shared<format_strategy, v210_strategy>(color_space, static_cast<uint8_t>(1));
}

spl::shared_ptr<format_strategy> create_hdr_v210_strategy(core::color_space color_space)
{
    return spl::make_shared<format_strategy, v210_strategy>(color_space, static_cast<uint8_t>(2));
}

}} // namespace caspar::decklink
