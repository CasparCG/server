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

#include <common/param.h>
#include <common/ptree.h>

namespace caspar { namespace decklink {

port_configuration parse_output_config(const boost::property_tree::wptree&  ptree,
                                       const core::video_format_repository& format_repository)
{
    port_configuration port_config;
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

configuration parse_xml_config(const boost::property_tree::wptree&  ptree,
                               const core::video_format_repository& format_repository)
{
    configuration config;

    auto duplex = ptree.get(L"duplex", L"default");
    if (duplex == L"full") {
        config.duplex = configuration::duplex_t::full_duplex;
    } else if (duplex == L"half") {
        config.duplex = configuration::duplex_t::half_duplex;
    }

    auto latency = ptree.get(L"latency", L"default");
    if (latency == L"low") {
        config.latency = configuration::latency_t::low_latency;
    } else if (latency == L"normal") {
        config.latency = configuration::latency_t::normal_latency;
    }

    auto wait_for_reference = ptree.get(L"wait-for-reference", L"auto");
    if (wait_for_reference == L"disable" || wait_for_reference == L"disabled") {
        config.wait_for_reference = configuration::wait_for_reference_t::disabled;
    } else if (wait_for_reference == L"enable" || wait_for_reference == L"enabled") {
        config.wait_for_reference = configuration::wait_for_reference_t::enabled;
    } else {
        config.wait_for_reference = configuration::wait_for_reference_t::automatic;
    }
    config.wait_for_reference_duration = ptree.get(L"wait-for-reference-duration", config.wait_for_reference_duration);

    config.primary = parse_output_config(ptree, format_repository);
    if (config.primary.device_index == -1)
        config.primary.device_index = 1;

    auto keyer = ptree.get(L"keyer", L"default");
    if (keyer == L"external") {
        config.keyer = configuration::keyer_t::external_keyer;
    } else if (keyer == L"internal") {
        config.keyer = configuration::keyer_t::internal_keyer;
    } else if (keyer == L"external_separate_device") {
        config.keyer = configuration::keyer_t::external_keyer;

        auto key_config         = config.primary; // Copy the primary config
        key_config.device_index = ptree.get(L"key-device", 0);
        if (key_config.device_index == 0) {
            key_config.device_index = config.primary.device_index + 1;
        }
        key_config.key_only = true;
        config.secondaries.push_back(key_config);
    }

    config.embedded_audio    = ptree.get(L"embedded-audio", config.embedded_audio);
    config.base_buffer_depth = ptree.get(L"buffer-depth", config.base_buffer_depth);

    if (ptree.get_child_optional(L"ports")) {
        for (auto& xml_port : ptree | witerate_children(L"ports") | welement_context_iteration) {
            ptree_verify_element_name(xml_port, L"port");

            port_configuration port_config = parse_output_config(xml_port.second, format_repository);

            config.secondaries.push_back(port_config);
        }
    }

    return config;
}

configuration parse_amcp_config(const std::vector<std::wstring>&     params,
                                const core::video_format_repository& format_repository)
{
    configuration config;

    if (params.size() > 1)
        config.primary.device_index = std::stoi(params.at(1));

    if (contains_param(L"INTERNAL_KEY", params)) {
        config.keyer = configuration::keyer_t::internal_keyer;
    } else if (contains_param(L"EXTERNAL_KEY", params)) {
        config.keyer = configuration::keyer_t::external_keyer;
    } else {
        config.keyer = configuration::keyer_t::default_keyer;
    }

    if (contains_param(L"FULL_DUPLEX", params)) {
        config.duplex = configuration::duplex_t::full_duplex;
    } else if (contains_param(L"HALF_DUPLEX", params)) {
        config.duplex = configuration::duplex_t::half_duplex;
    }

    if (contains_param(L"LOW_LATENCY", params)) {
        config.latency = configuration::latency_t::low_latency;
    }

    config.embedded_audio   = contains_param(L"EMBEDDED_AUDIO", params);
    config.primary.key_only = contains_param(L"KEY_ONLY", params);

    return config;
}

}} // namespace caspar::decklink