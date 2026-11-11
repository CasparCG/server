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
 */

#pragma once

#include <common/timespan.h>

#include <core/fwd.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <optional>
#include <string>
#include <vector>

namespace caspar { namespace screen {

enum class stretch
{
    none,
    uniform,
    fill,
    uniform_to_fill
};

struct configuration
{
    enum class aspect_ratio
    {
        aspect_4_3 = 0,
        aspect_16_9,
        aspect_invalid,
    };

    enum class colour_spaces
    {
        RGB               = 0,
        datavideo_full    = 1,
        datavideo_limited = 2
    };

    std::wstring    name          = L"Screen consumer";
    int             screen_index  = 0;
    int             screen_x      = 0;
    int             screen_y      = 0;
    int             screen_width  = 0;
    int             screen_height = 0;
    screen::stretch stretch       = screen::stretch::fill;
    bool            windowed      = true;
    bool            key_only      = false;
    bool            sbs_key       = false;
    aspect_ratio    aspect        = aspect_ratio::aspect_invalid;
#ifdef __APPLE__
    // macOS: queue frames to the display refresh, otherwise frames whose arrival phase is close to
    // a refresh are shown twice / skipped (the channel and the display run on separate clocks).
    bool vsync = true;
#else
    bool vsync = false;
#endif
    bool            interactive   = true;
    bool            borderless    = false;
    bool            always_on_top = false;
    colour_spaces   colour_space  = colour_spaces::RGB;
    bool            high_bitdepth = false;
    bool            gpu_texture   = false;

    timespan delay;
};

// Returns an empty optional when the params do not describe a screen consumer.
std::optional<configuration> parse_params(const std::vector<std::wstring>& params,
                                          const core::channel_info&        channel_info);

configuration parse_ptree(const boost::property_tree::wptree& ptree, const core::channel_info& channel_info);

}} // namespace caspar::screen
