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

#include "config.h"

namespace caspar { namespace decklink {

output_configuration parse_output_config(const boost::property_tree::wptree&  ptree,
                                         const core::video_format_repository& format_repository)
{
    output_configuration port_config;
    port_config.device_index = ptree.get(L"device", -1);
    port_config.key_only     = ptree.get(L"key-only", port_config.key_only);

    auto format_desc_str = ptree.get(L"video-mode", L"");
    if (!format_desc_str.empty()) {
        auto format_desc = format_repository.find(format_desc_str);
        if (format_desc.format == core::video_format::invalid || format_desc.format == core::video_format::custom)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video-mode: " + format_desc_str));
        port_config.format = format_desc;
    }

    auto subregion_tree = ptree.get_child_optional(L"subregion");
    if (subregion_tree) {
        port_config.src_x    = subregion_tree->get(L"src-x", port_config.src_x);
        port_config.src_y    = subregion_tree->get(L"src-y", port_config.src_y);
        port_config.dest_x   = subregion_tree->get(L"dest-x", port_config.dest_x);
        port_config.dest_y   = subregion_tree->get(L"dest-y", port_config.dest_y);
        port_config.region_w = subregion_tree->get(L"width", port_config.region_w);
        port_config.region_h = subregion_tree->get(L"height", port_config.region_h);
    }

    return port_config;
}

}} // namespace caspar::decklink