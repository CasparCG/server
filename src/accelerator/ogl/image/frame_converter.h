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

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>

#include "../util/device.h"

namespace caspar::accelerator::ogl {

class ogl_frame_converter
    : public core::frame_converter
    , public std::enable_shared_from_this<ogl_frame_converter>
{
  public:
    ogl_frame_converter(const spl::shared_ptr<device>& ogl);
    ogl_frame_converter(const ogl_frame_converter&) = delete;

    ~ogl_frame_converter() override = default;

    ogl_frame_converter& operator=(const ogl_frame_converter&) = delete;

    // core::mutable_frame create_frame(const void* video_stream_tag, const core::pixel_format_desc& desc) override;

    // core::draw_frame convert_to_rgba(const core::mutable_frame& frame) override;

    std::shared_future<array<const std::uint8_t>> convert_from_rgba(const core::const_frame&   frame,
                                                                    core::encoded_frame_format format,
                                                                    bool                       key_only,
                                                                    bool                       straighten) override;

    common::bit_depth get_frame_bitdepth(const core::const_frame& frame) override;

  private:
    const spl::shared_ptr<device> ogl_;
};

} // namespace caspar::accelerator::ogl