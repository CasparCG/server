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

#include <core/video_format.h>

namespace caspar { namespace decklink {

struct output_configuration
{
    int  device_index = 1;
    bool key_only     = false;

    core::video_format_desc format;
    int                     src_x    = 0;
    int                     src_y    = 0;
    int                     dest_x   = 0;
    int                     dest_y   = 0;
    int                     region_w = 0;
    int                     region_h = 0;

    [[nodiscard]] bool has_subregion_geometry() const
    {
        return src_x != 0 || src_y != 0 || region_w != 0 || region_h != 0 || dest_x != 0 || dest_y != 0;
    }
};

struct configuration
{
    enum class keyer_t
    {
        internal_keyer,
        external_keyer,
        external_separate_device_keyer, // TODO - remove?
        default_keyer = external_keyer
    };

    enum class duplex_t
    {
        none,
        half_duplex,
        full_duplex,
        default_duplex = none
    };

    enum class latency_t
    {
        low_latency,
        normal_latency,
        default_latency = normal_latency
    };

    // int       device_index      = 1;
    // int       key_device_idx    = 0;
    bool      embedded_audio    = false;
    keyer_t   keyer             = keyer_t::default_keyer;
    duplex_t  duplex            = duplex_t::default_duplex;
    latency_t latency           = latency_t::default_latency;
    int       base_buffer_depth = 3;

    output_configuration              main;
    std::vector<output_configuration> children;

    [[nodiscard]] int buffer_depth() const
    {
        return base_buffer_depth + (latency == latency_t::low_latency ? 0 : 1) +
               (embedded_audio ? 1 : 0); // TODO: Do we need this?
    }

    // int key_device_index() const { return key_device_idx == 0 ? device_index + 1 : key_device_idx; }
};

output_configuration parse_output_config(const boost::property_tree::wptree&  ptree,
                                         const core::video_format_repository& format_repository);

}} // namespace caspar::decklink