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

#include <boost/property_tree/ptree.hpp>

#include <core/consumer/channel_info.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

namespace caspar { namespace decklink {

struct port_configuration
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

struct hdr_meta_configuration
{
    float min_dml  = 0.005f;
    float max_dml  = 1000.0f;
    float max_fall = 100.0f;
    float max_cll  = 1000.0f;
};

struct configuration
{
    enum class keyer_t
    {
        internal_keyer,
        external_keyer,
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

    enum class wait_for_reference_t
    {
        automatic,
        enabled,
        disabled,
    };

    bool                 embedded_audio              = false;
    keyer_t              keyer                       = keyer_t::default_keyer;
    duplex_t             duplex                      = duplex_t::default_duplex;
    latency_t            latency                     = latency_t::default_latency;
    wait_for_reference_t wait_for_reference          = wait_for_reference_t::automatic;
    int                  wait_for_reference_duration = 10; // seconds
    int                  base_buffer_depth           = 3;
    bool                 hdr                         = false;

    port_configuration              primary;
    std::vector<port_configuration> secondaries;

    core::color_space      color_space = core::color_space::bt709;
    hdr_meta_configuration hdr_meta;

    [[nodiscard]] int buffer_depth() const
    {
        return base_buffer_depth + (latency == latency_t::low_latency ? 0 : 1) +
               (embedded_audio ? 1 : 0); // TODO: Do we need this?
    }

    // int key_device_index() const { return key_device_idx == 0 ? device_index + 1 : key_device_idx; }
};

configuration parse_xml_config(const boost::property_tree::wptree&  ptree,
                               const core::video_format_repository& format_repository,
                               const core::channel_info&            channel_info);

configuration parse_amcp_config(const std::vector<std::wstring>&     params,
                                const core::video_format_repository& format_repository,
                                const core::channel_info&            channel_info);

}} // namespace caspar::decklink
