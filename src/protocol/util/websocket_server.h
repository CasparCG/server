/*
 * Copyright (c) 2024 CasparCG (www.casparcg.com).
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

#include "protocol_strategy.h"

#include <common/memory.h>
#include <core/monitor/monitor.h>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace caspar { namespace IO {

/**
 * WebSocket server that handles both AMCP protocol commands and monitor data streaming.
 */
class websocket_server
{
  public:
    // Constructor for AMCP + Monitor (legacy)
    websocket_server(std::shared_ptr<boost::asio::io_context>       context,
                     const protocol_strategy_factory<wchar_t>::ptr& amcp_protocol_factory,
                     uint16_t                                       amcp_port,
                     uint16_t                                       monitor_port);

    // Constructor for AMCP only (new)
    websocket_server(std::shared_ptr<boost::asio::io_context>       context,
                     const protocol_strategy_factory<wchar_t>::ptr& amcp_protocol_factory,
                     uint16_t                                       amcp_port);

    ~websocket_server();

    void start();
    void stop();

    // Send monitor data to all connected monitor clients
    void send_monitor_data(const core::monitor::state& state);

    // Add lifecycle factory for AMCP clients
    void add_client_lifecycle_object_factory(
        const std::function<std::pair<std::wstring, std::shared_ptr<void>>(const std::string& address)>& factory);

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;

    websocket_server(const websocket_server&)            = delete;
    websocket_server& operator=(const websocket_server&) = delete;
};

}} // namespace caspar::IO