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

#include "image_converter.h"

#include <common/except.h>

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
}

namespace caspar::image {

bool is_frame_compatible_with_mixer(const std::shared_ptr<AVFrame>& src) {
    std::vector<int> data_map;
    auto sample_pix_desc =
            ffmpeg::pixel_format_desc(
                    static_cast<AVPixelFormat>(src->format), src->width, src->height, data_map, core::color_space::bt709);
    return sample_pix_desc.format != core::pixel_format::invalid;
}

std::shared_ptr<AVFrame> convert_image_frame(const std::shared_ptr<AVFrame>& src, AVPixelFormat pixFmt) {
    if (src->format == pixFmt) return src;

    std::shared_ptr<SwsContext> sws;
    sws.reset(sws_getContext(
                      src->width, src->height, static_cast<AVPixelFormat>(src->format), src->width, src->height, pixFmt, 0, nullptr, nullptr, nullptr),
              [](SwsContext* ptr) { sws_freeContext(ptr); });

    if (!sws) {
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create SwsContext"));
    }

    auto dest = ffmpeg::alloc_frame();
    dest->sample_aspect_ratio = src->sample_aspect_ratio;
    dest->width               = src->width;
    dest->height              = src->height;
    dest->format              = pixFmt;
    dest->colorspace          = AVCOL_SPC_BT709;
    av_frame_get_buffer(dest.get(), 64);

    sws_scale_frame(sws.get(), dest.get(), src.get());

    return dest;
}

}