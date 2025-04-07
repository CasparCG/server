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
#include "frame_converter.h"
#include "../util/convert_description.h"
#include "../util/texture.h"

#include <common/except.h>

#include <boost/functional/hash.hpp>

namespace caspar::accelerator::ogl {

std::size_t hash_convertion_format(const caspar::core::frame_conversion_format &s) noexcept {
    std::size_t result = 0;
    boost::hash_combine(result, s.format);
    boost::hash_combine(result, s.width);
    boost::hash_combine(result, s.height);
    boost::hash_combine(result, s.key_only);
    boost::hash_combine(result, s.straight_alpha);

    boost::hash_combine(result, s.region.src_x);
    boost::hash_combine(result, s.region.src_y);
    boost::hash_combine(result, s.region.dest_x);
    boost::hash_combine(result, s.region.dest_y);
    boost::hash_combine(result, s.region.w);
    boost::hash_combine(result, s.region.h);
    return result;
}


ogl_frame_converter::ogl_frame_converter(const spl::shared_ptr<device>& ogl)
    : ogl_(ogl)
{
}

std::shared_future<array<const std::uint8_t>>
ogl_frame_converter::convert_to_buffer(const core::const_frame&         frame,
                                       const core::frame_conversion_format& format)
{
    auto tex_ptr = std::any_cast<std::shared_ptr<ogl_texture_and_buffer_cache>>(frame.image_ptr());
    if (!tex_ptr)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("No texture inside frame"));

    // Future: sanitise the subregion geometry, to ensure it is sane and in the simplest form
    size_t cache_key = hash_convertion_format(format);

    // Note: This cache is a bit race prone, but is memory safe. At worst we will schedule the download of the same
    // buffer multiple times rather than reusing the one. But it will do so safely.
    auto cached_download = tex_ptr->buffer_cache.find(cache_key);
    if (cached_download != tex_ptr->buffer_cache.end()) {
        return cached_download->second;
    }

    int          buffer_size    = 0;
    unsigned int x_count        = 0;
    unsigned int y_count        = 0;
    int          words_per_line = 0;

    switch (format.format) {
//        case core::frame_conversion_format::pixel_format::rgba8:
        case core::frame_conversion_format::pixel_format::bgra8:
            x_count        = frame.width();
            y_count        = frame.height();
            buffer_size    = frame.width() * frame.height() * 4;
            words_per_line = frame.width();
            break;

//        case core::frame_conversion_format::pixel_format::rgba16:
//        case core::frame_conversion_format::pixel_format::bgra16:
//            x_count        = frame.width();
//            y_count        = frame.height();
//            buffer_size    = frame.width() * frame.height() * 8;
//            words_per_line = frame.width() * 2;
//
//            break;
//        case core::frame_conversion_format::pixel_format::decklink_v210:
//            auto row_blocks = ((frame.width() + 47) / 48);
//            auto row_bytes  = row_blocks * 128;
//
//            // TODO - result must be 128byte aligned. can that be guaranteed here?
//            buffer_size    = row_bytes * frame.height();
//            x_count        = row_blocks * 8;
//            y_count        = frame.height();
//            words_per_line = row_blocks * 32;
//            break;
        default:
            // Handled in guard below
            break;
    }

    if (buffer_size == 0 || x_count == 0 || y_count == 0 || words_per_line == 0) {
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Unknown encoded frame format"));
    }

    x_count = (x_count + 31) / 32;
    y_count = (y_count + 31) / 32;

    convert_from_texture_description description{};
    description.target_format  = format.format;
    description.is_16_bit      = tex_ptr->gl_texture->depth() == common::bit_depth::bit16;
    description.width          = frame.width();
    description.height         = frame.height();
    description.words_per_line = words_per_line;
    description.key_only       = format.key_only;
    description.straighten     = format.straight_alpha;

    // Start download
    auto new_download = ogl_->convert_from_texture(tex_ptr->gl_texture, buffer_size, description, x_count, y_count).share();

    tex_ptr->buffer_cache.emplace(cache_key, new_download);

    return new_download;
}

common::bit_depth ogl_frame_converter::get_frame_bitdepth(const core::const_frame& frame)
{
    auto texture_ptr = std::any_cast<std::shared_ptr<ogl_texture_and_buffer_cache>>(frame.image_ptr());
    if (!texture_ptr) {
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("No texture inside frame"));
    }

    return texture_ptr->gl_texture->depth();
}

} // namespace caspar::accelerator::ogl