/*
 * Copyright (c) 2018 Norsk rikskringkasting AS
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

#include "amcp_command_repository.h"
#include "amcp_shared.h"
#include <accelerator/accelerator.h>
#include <common/forward.h>
#include <core/consumer/frame_consumer.h>
#include <future>
#include <utility>

FORWARD3(caspar, protocol, osc, class client);

namespace caspar { namespace protocol { namespace amcp {

struct amcp_command_static_context
{
    const core::video_format_repository                        format_repository;
    const spl::shared_ptr<core::cg_producer_registry>          cg_registry;
    const spl::shared_ptr<const core::frame_producer_registry> producer_registry;
    const spl::shared_ptr<const core::frame_consumer_registry> consumer_registry;
    const std::shared_ptr<amcp_command_repository>             parser;
    std::weak_ptr<accelerator::accelerator_device>             ogl_device;

    amcp_command_static_context(core::video_format_repository                               format_repository,
                                const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
                                const spl::shared_ptr<const core::frame_producer_registry>& producer_registry,
                                const spl::shared_ptr<const core::frame_consumer_registry>& consumer_registry,
                                std::shared_ptr<amcp_command_repository>                    parser,
                                std::weak_ptr<accelerator::accelerator_device>              ogl_device)
        : format_repository(std::move(format_repository))
        , cg_registry(cg_registry)
        , producer_registry(producer_registry)
        , consumer_registry(consumer_registry)
        , parser(std::move(parser))
        , ogl_device(std::move(ogl_device))
    {
    }
};

struct command_context
{
    std::shared_ptr<amcp_command_static_context>        static_context;
    const spl::shared_ptr<std::vector<channel_context>> channels;
    const std::wstring                                  client_address;
    const channel_context                               channel;
    const int                                           channel_index;
    const int                                           layer_id;
    std::vector<std::wstring>                           parameters;

    int layer_index(int default_ = 0) const { return layer_id == -1 ? default_ : layer_id; }

    command_context(std::shared_ptr<amcp_command_static_context>        static_context,
                    const spl::shared_ptr<std::vector<channel_context>> channels,
                    const std::wstring&                                 client_address,
                    channel_context                                     channel,
                    int                                                 channel_index,
                    int                                                 layer_id)
        : static_context(std::move(static_context))
        , channels(std::move(channels))
        , client_address(client_address)
        , channel(std::move(channel))
        , channel_index(channel_index)
        , layer_id(layer_id)
    {
    }
};

}}} // namespace caspar::protocol::amcp
