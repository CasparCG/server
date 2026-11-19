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

#include "fwd.h"
#include "video_format.h"

#include "frame/pixel_format.h"

#include "monitor/monitor.h"

#include <common/memory.h>

#include <boost/signals2.hpp>

#include <functional>
#include <optional>

namespace caspar { namespace core {

enum route_mode
{
    foreground,
    background,
    next, // background if any, otherwise foreground
};

struct route_id
{
    int        index;
    route_mode mode;

    bool const operator==(const route_id& o) { return index == o.index && mode == o.mode; }
};

struct route
{
    route()             = default;
    route(const route&) = delete;
    route(route&&)      = default;

    route& operator=(const route&) = delete;
    route& operator=(route&&)      = default;

    boost::signals2::signal<void(class draw_frame, class draw_frame)> signal;
    video_format_desc                                                 format_desc;
    std::wstring                                                      name;
};

class video_channel final
{
    video_channel(const video_channel&);
    video_channel& operator=(const video_channel&);

  public:
    explicit video_channel(int                                       index,
                           const video_format_desc&                  format_desc,
                           color_space                               default_color_space,
                           std::unique_ptr<image_mixer>              image_mixer,
                           std::function<void(core::monitor::state)> on_tick);
    ~video_channel();

    core::monitor::state state() const;

    const std::shared_ptr<core::stage>& stage() const;
    std::shared_ptr<core::stage>&       stage();
    const core::mixer&                  mixer() const;
    core::mixer&                        mixer();
    const core::output&                 output() const;
    core::output&                       output();

    spl::shared_ptr<core::frame_factory> frame_factory();

    int index() const;

    [[nodiscard]] channel_info get_consumer_channel_info() const;

    std::shared_ptr<core::route> route(int index = -1, route_mode mode = route_mode::foreground);
    std::shared_ptr<class audio_tap> audio_tap(int layer, bool post, int maximum_read_samples = 0);
    void                             configure_audio_matrix(int                                                           destination,
                                                            const std::optional<std::vector<struct audio_matrix_source>>& sources,
                                                            const std::vector<spl::shared_ptr<video_channel>>&            channels);
    void                             clear_audio_matrix();
    std::wstring                     audio_matrix_state() const;

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;
};

}} // namespace caspar::core
