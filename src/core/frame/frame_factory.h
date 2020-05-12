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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#ifdef WIN32
#include <common/forward.h>
#include <memory>
FORWARD3(caspar, accelerator, d3d, class d3d_texture2d);
#endif

namespace caspar { namespace core {

class frame_factory
{
  public:
    frame_factory()        = default;
    frame_factory& operator=(const frame_factory&) = delete;
    virtual ~frame_factory()                       = default;

    frame_factory(const frame_factory&) = delete;

    virtual class mutable_frame create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;

#ifdef WIN32
    virtual class const_frame import_d3d_texture(const void* video_stream_tag,
                                                 const std::shared_ptr<accelerator::d3d::d3d_texture2d>& d3d_texture,
                                                 bool vflip = false) = 0;
#endif
};

}} // namespace caspar::core
