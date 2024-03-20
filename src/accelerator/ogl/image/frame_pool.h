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

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include "image_mixer.h"

namespace caspar::accelerator::ogl {

struct pooled_buffer
{
    std::vector<caspar::array<std::uint8_t>> buffers;
    std::atomic<bool>                        in_use;

    std::any data;
};

class frame_pool : public core::frame_pool
{
  public:
    frame_pool(std::shared_ptr<frame_factory_gl> frame_factory, const void* tag, core::pixel_format_desc desc);
    frame_pool(const frame_pool&) = delete;

    ~frame_pool() override = default;

    frame_pool& operator=(const frame_pool&) = delete;

    std::pair<core::mutable_frame, std::any&> create_frame() override;

    void for_each(const std::function<void(std::any& data)>& fn) override;

    void                                         pixel_format(core::pixel_format_desc desc) override;
    [[nodiscard]] const core::pixel_format_desc& pixel_format() const override;

  private:
    const std::shared_ptr<frame_factory_gl> frame_factory_;
    const void*                             tag_;
    core::pixel_format_desc                 desc_;

    std::vector<std::shared_ptr<pooled_buffer>> pool_;
};

} // namespace caspar::accelerator::ogl
