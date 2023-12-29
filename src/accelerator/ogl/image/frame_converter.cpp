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
#include "../util/texture.h"

#include <core/frame/pixel_format.h>

#include <common/except.h>
#include <common/future.h>

namespace caspar::accelerator::ogl {

ogl_frame_converter::ogl_frame_converter(const spl::shared_ptr<device>& ogl)
    : ogl_(ogl)
{
}

core::mutable_frame ogl_frame_converter::create_frame(const void* tag, const core::pixel_format_desc& desc)
{
    std::vector<array<std::uint8_t>> image_data;
    for (auto& plane : desc.planes) {
        image_data.push_back(ogl_->create_array(plane.size));
    }

    using future_texture = std::shared_future<std::shared_ptr<texture>>;

    std::weak_ptr<ogl_frame_converter> weak_self = shared_from_this();
    return core::mutable_frame(
        tag,
        std::move(image_data),
        array<int32_t>{},
        desc,
        [weak_self, desc](std::vector<array<const std::uint8_t>> image_data) -> boost::any {
            // TODO - replace this
            auto self = weak_self.lock();
            if (!self) {
                return boost::any{};
            }
            std::vector<future_texture> textures;
            for (int n = 0; n < static_cast<int>(desc.planes.size()); ++n) {
                textures.emplace_back(self->ogl_->copy_async(
                    image_data[n], desc.planes[n].width, desc.planes[n].height, desc.planes[n].stride, desc.planes[n].depth));
            }
            return std::make_shared<decltype(textures)>(std::move(textures));
        });
}

core::draw_frame ogl_frame_converter::convert_frame(const core::mutable_frame& frame)
{
    // TODO
    return core::draw_frame{};
}

std::shared_future<std::vector<array<const std::uint8_t>>>
ogl_frame_converter::convert_from_rgba(const core::const_frame& frame, const core::encoded_frame_format format)
{
    std::vector<array<const std::uint8_t>> buffers;
    int                                    x_count = 0;
    int                                    y_count = 0;
    int words_per_line = 0;

    switch (format) {
        case core::encoded_frame_format::decklink_v210:
            auto row_blocks = ((frame.width() + 47) / 48);
            auto row_bytes  = row_blocks * 128;

            // TODO - result must be 128byte aligned. can that be guaranteed here?
            buffers.emplace_back(ogl_->create_array(row_bytes * frame.height()));
            x_count = row_blocks * 8;
            y_count = frame.height();
            words_per_line = row_blocks * 32;
            break;
    }

    if (buffers.empty() || x_count == 0 || y_count == 0) {
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Unknown encoded frame format"));
    }

    auto texture_ptr = boost::any_cast<std::shared_ptr<texture>>(frame.opaque());
    if (!texture_ptr) {
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("No texture inside frame!"));
    }

    convert_from_texture_description description{};
    description.width = frame.width();
    description.height = frame.height();
    description.words_per_line = words_per_line;

    auto future_conversion =
        ogl_->convert_from_texture(texture_ptr, buffers, description, x_count, y_count);

    return std::async(std::launch::deferred,
                      [buffers = std::move(buffers), future_conversion = std::move(future_conversion)]() mutable {
                          future_conversion.get();

                          return std::move(buffers);
                      });
}

} // namespace caspar::accelerator::ogl