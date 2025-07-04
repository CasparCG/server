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

#include <boost/asio/io_context.hpp>

#include <common/memory.h>
#include <core/monitor/monitor.h>

#include <chrono>
#include <string>

namespace caspar { namespace protocol { namespace websocket {

struct monitor_message
{
    enum class type
    {
        full_state,
        delta,
        heartbeat
    };

    type                      message_type;
    std::chrono::milliseconds timestamp;
    std::string               json_data;
};

struct connection_config
{
    std::chrono::seconds full_state_interval{0}; // Disable periodic full state (0 = disabled)
    bool                 send_heartbeat{true};
    std::chrono::seconds heartbeat_interval{10}; // Send heartbeat every 10 seconds
};

class websocket_monitor_client
{
    websocket_monitor_client(const websocket_monitor_client&)            = delete;
    websocket_monitor_client& operator=(const websocket_monitor_client&) = delete;

  public:
    explicit websocket_monitor_client(std::shared_ptr<boost::asio::io_context> context);

    websocket_monitor_client(websocket_monitor_client&&);

    /**
     * Get a subscription token that ensures that WebSocket monitor messages are sent to the
     * given connection as long as the token is alive. It will stop sending when
     * the token is dropped unless another token to the same connection has
     * previously been checked out.
     *
     * @param connection_id The unique identifier for the WebSocket connection.
     *
     * @return The token. It is ok for the token to outlive the client
     */
    std::shared_ptr<void> get_subscription_token(const std::string& connection_id);

    ~websocket_monitor_client();

    websocket_monitor_client& operator=(websocket_monitor_client&&);

    void send(const core::monitor::state& state);

    // Called by WebSocket server to register/unregister connections
    void add_connection(const std::string&                      connection_id,
                        std::function<void(const std::string&)> send_callback,
                        const connection_config&                config = {});
    void remove_connection(const std::string& connection_id);

    // Force send full state to all connections (useful for debugging)
    void force_full_state();

    // Force send full state to a specific connection
    void request_full_state(const std::string& connection_id);

    // Force disconnect all connections (for clean shutdown)
    void force_disconnect_all();

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}}} // namespace caspar::protocol::websocket