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

#include <core/frame/pixel_format.h>

#include <common/except.h>

namespace caspar::accelerator::ogl {

ogl_frame_converter::ogl_frame_converter(const spl::shared_ptr<device>& ogl)
    : ogl_(ogl)
{
}

std::shared_future<array<const std::uint8_t>>
ogl_frame_converter::convert_to_buffer(const core::const_frame&         frame,
                                       const core::frame_conversion_format& format)
{
    if (format.format != core::frame_conversion_format::pixel_format::bgra8)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("format not implemented"));
    if (format.key_only)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("key_only not implemented"));
    if (format.straight_alpha)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("straight_alpha not implemented"));

    auto tex_ptr = std::any_cast<std::shared_ptr<ogl_texture_and_buffer_cache>>(frame.image_ptr());
    if (!tex_ptr)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("No texture inside frame"));

    uint8_t cache_key = 0; // Future: this should be a unique key derived from `frame_conversion_format`

    // Note: This cache is a bit race prone, but is memory safe. At worst we will schedule the download of the same
    // buffer multiple times rather than reusing the one. But it will do so safely.
    auto cached_download = tex_ptr->buffer_cache.find(cache_key);
    if (cached_download != tex_ptr->buffer_cache.end()) {
        return cached_download->second;
    }

    // Download unmodified for now
    auto new_download = ogl_->copy_async(tex_ptr->gl_texture).share();
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