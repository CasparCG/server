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

#include <common/array.h>
#include <common/memory.h>

#include <core/frame/frame.h>
#include <core/mixer/image/image_mixer.h>
#include <core/video_format.h>

#include <future>

namespace caspar { namespace accelerator { namespace ogl {

class image_mixer final : public core::image_mixer
{
  public:
    image_mixer(const spl::shared_ptr<class device>& ogl, int channel_id);
    image_mixer(const image_mixer&) = delete;

    ~image_mixer();

    image_mixer& operator=(const image_mixer&) = delete;

    std::future<array<const std::uint8_t>> operator()(const core::video_format_desc& format_desc) override;
    core::mutable_frame                    create_frame(const void* tag, const core::pixel_format_desc& desc) override;
#ifdef WIN32
    core::const_frame
    import_d3d_texture(const void* tag, const std::shared_ptr<d3d::d3d_texture2d>& d3d_texture, bool vflip) override;
#endif

    // core::image_mixer

    void push(const core::frame_transform& frame) override;
    void visit(const core::const_frame& frame) override;
    void pop() override;

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::ogl
