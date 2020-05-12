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

#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_visitor.h>

#include <cstdint>
#include <future>

namespace caspar { namespace core {

class image_mixer
    : public frame_visitor
    , public frame_factory
{
    image_mixer(const image_mixer&);
    image_mixer& operator=(const image_mixer&);

  public:
    image_mixer() {}
    virtual ~image_mixer() {}

    void push(const struct frame_transform& frame) override = 0;
    void visit(const class const_frame& frame) override     = 0;
    void pop() override                                     = 0;

    virtual std::future<array<const uint8_t>> operator()(const struct video_format_desc& format_desc) = 0;

    class mutable_frame create_frame(const void* tag, const struct pixel_format_desc& desc) override = 0;

#ifdef WIN32
    class const_frame import_d3d_texture(const void*                                             tag,
                                         const std::shared_ptr<accelerator::d3d::d3d_texture2d>& d3d_texture,
                                         bool                                                    vflip) override = 0;
#endif
};

}} // namespace caspar::core
