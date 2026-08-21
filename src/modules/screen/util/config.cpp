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

#include "config.h"

#include <common/bit_depth.h>
#include <common/log.h>
#include <common/param.h>

#include <core/consumer/channel_info.h>

#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace screen {

configuration parse_consumer_params(const std::vector<std::wstring>& params, const core::channel_info& channel_info)
{
    configuration config;

    config.high_bitdepth = (channel_info.depth != common::bit_depth::bit8);

    if (params.size() > 1) {
        try {
            config.screen_index = std::stoi(params.at(1));
        } catch (...) {
        }
    }

    config.windowed    = !contains_param(L"FULLSCREEN", params);
    config.key_only    = contains_param(L"KEY_ONLY", params);
    config.sbs_key     = contains_param(L"SBS_KEY", params);
    config.interactive = !contains_param(L"NON_INTERACTIVE", params);
    config.borderless  = contains_param(L"BORDERLESS", params);

    if (contains_param(L"NAME", params))
        config.name = get_param(L"NAME", params);
    if (contains_param(L"X", params))
        config.screen_x = get_param(L"X", params, 0);
    if (contains_param(L"Y", params))
        config.screen_y = get_param(L"Y", params, 0);
    if (contains_param(L"WIDTH", params))
        config.screen_width = get_param(L"WIDTH", params, 0);
    if (contains_param(L"HEIGHT", params))
        config.screen_height = get_param(L"HEIGHT", params, 0);
    if (contains_param(L"DELAY", params)) {
        config.delay = timespan{u8(get_param(L"DELAY", params, L""))};
    }

    if (config.sbs_key && config.key_only) {
        CASPAR_LOG(warning) << L" Key-only not supported with configuration of side-by-side fill and key. Ignored.";
        config.key_only = false;
    }

    return config;
}

configuration parse_preconfigured_consumer(const boost::property_tree::wptree& ptree,
                                           const core::channel_info&           channel_info)
{
    configuration config;

    config.high_bitdepth = (channel_info.depth != common::bit_depth::bit8);

    config.name          = ptree.get(L"name", config.name);
    config.screen_index  = ptree.get(L"device", config.screen_index + 1) - 1;
    config.screen_x      = ptree.get(L"x", config.screen_x);
    config.screen_y      = ptree.get(L"y", config.screen_y);
    config.screen_width  = ptree.get(L"width", config.screen_width);
    config.screen_height = ptree.get(L"height", config.screen_height);
    config.windowed      = ptree.get(L"windowed", config.windowed);
    config.key_only      = ptree.get(L"key-only", config.key_only);
    config.sbs_key       = ptree.get(L"sbs-key", config.sbs_key);
    config.vsync         = ptree.get(L"vsync", config.vsync);
    config.interactive   = ptree.get(L"interactive", config.interactive);
    config.borderless    = ptree.get(L"borderless", config.borderless);
    config.always_on_top = ptree.get(L"always-on-top", config.always_on_top);

    config.delay = timespan{u8(ptree.get(L"delay", L"0"))};

    auto colour_space_value = ptree.get(L"colour-space", L"RGB");
    config.colour_space     = configuration::colour_spaces::RGB;
    if (colour_space_value == L"datavideo-full")
        config.colour_space = configuration::colour_spaces::datavideo_full;
    else if (colour_space_value == L"datavideo-limited")
        config.colour_space = configuration::colour_spaces::datavideo_limited;

    if (config.sbs_key && config.key_only) {
        CASPAR_LOG(warning) << L" Key-only not supported with configuration of side-by-side fill and key. Ignored.";
        config.key_only = false;
    }
    if ((config.colour_space == configuration::colour_spaces::datavideo_full ||
         config.colour_space == configuration::colour_spaces::datavideo_limited) &&
        config.sbs_key) {
        CASPAR_LOG(warning) << L" Side-by-side fill and key not supported for DataVideo TC100/TC200. Ignored.";
        config.sbs_key = false;
    }
    if ((config.colour_space == configuration::colour_spaces::datavideo_full ||
         config.colour_space == configuration::colour_spaces::datavideo_limited) &&
        config.key_only) {
        CASPAR_LOG(warning) << L" Key only not supported for DataVideo TC100/TC200. Ignored.";
        config.key_only = false;
    }

    auto stretch_str = ptree.get(L"stretch", L"fill");
    if (stretch_str == L"none")
        config.stretch = screen::stretch::none;
    else if (stretch_str == L"uniform")
        config.stretch = screen::stretch::uniform;
    else if (stretch_str == L"uniform_to_fill")
        config.stretch = screen::stretch::uniform_to_fill;

    auto aspect_str = ptree.get(L"aspect-ratio", L"default");
    if (aspect_str == L"16:9")
        config.aspect = configuration::aspect_ratio::aspect_16_9;
    else if (aspect_str == L"4:3")
        config.aspect = configuration::aspect_ratio::aspect_4_3;

    return config;
}

}} // namespace caspar::screen
