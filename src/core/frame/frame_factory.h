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

#include <any>

namespace caspar { namespace core {

class frame_pool {
  public:
    frame_pool()                                = default;
    frame_pool& operator=(const frame_pool&) = delete;
    virtual ~frame_pool()                       = default;

    frame_pool(const frame_pool&) = delete;

    virtual std::pair<class mutable_frame, std::any&> create_frame() = 0;

    virtual void for_each(const std::function<void(std::any& data)>& fn) = 0;
};

class frame_factory
{
  public:
    frame_factory()                                = default;
    frame_factory& operator=(const frame_factory&) = delete;
    virtual ~frame_factory()                       = default;

    frame_factory(const frame_factory&) = delete;

    virtual class mutable_frame create_frame(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;

    virtual std::unique_ptr<frame_pool> create_frame_pool(const void* video_stream_tag, const struct pixel_format_desc& desc) = 0;
};

}} // namespace caspar::core
