/*
 * Copyright (c) 2025 Sveriges Television AB <info@casparcg.com>
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
 */

#pragma once

#include <core/monitor/monitor.h>

#include <boost/asio/io_context.hpp>
#include <functional>
#include <memory>
#include <string>

namespace caspar { namespace protocol { namespace websocket {

class websocket_monitor_client
{
  public:
    explicit websocket_monitor_client(std::shared_ptr<boost::asio::io_context> context);
    ~websocket_monitor_client();

    // Simplified connection management
    void add_connection(const std::string& connection_id, std::function<void(const std::string&)> send_callback);
    void remove_connection(const std::string& connection_id);

    // Simple broadcast to all connections
    void send(const core::monitor::state& state);

    // Force disconnect all (for shutdown)
    void force_disconnect_all();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::protocol::websocket