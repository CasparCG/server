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

namespace caspar::accelerator::ogl {

ogl_frame_converter::ogl_frame_converter(const spl::shared_ptr<device>& ogl)
    : ogl_(ogl)
{
}

core::mutable_frame ogl_frame_converter::create_frame(const void* tag, const core::pixel_format_desc& desc)
{

    std::vector<array<std::uint8_t>> image_data;
    for (auto& plane : desc.planes) {
        image_data.push_back(ogl_->create_array(plane.size, common::bit_depth::bit16)); // TODO: Depth
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
                    image_data[n], desc.planes[n].width, desc.planes[n].height, desc.planes[n].stride));
            }
            return std::make_shared<decltype(textures)>(std::move(textures));
        });
}

core::draw_frame ogl_frame_converter::convert_frame(const core::mutable_frame& frame)
{
    // TODO
    return core::draw_frame{};
}

} // namespace caspar::accelerator::ogl