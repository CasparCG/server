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
#include <regex>
#include <string>
#include <vector>

namespace caspar { namespace protocol { namespace websocket {

// Array indexing support
struct array_filter
{
    bool has_filter   = false;
    bool single_index = false;
    int  start        = 0;
    int  end          = -1; // -1 means no end (single index or all)
};

// Simple wildcard path matcher with array indexing support
class path_matcher
{
  public:
    explicit path_matcher(const std::string& pattern);

    // Check if a path matches this pattern
    bool matches(const std::string& path) const;

    // Get the original pattern
    const std::string& pattern() const { return pattern_; }

    // Check if this pattern has array filtering
    bool has_array_filter() const;

    // Apply array filtering to values (returns filtered values)
    caspar::core::monitor::vector_t apply_array_filter(const caspar::core::monitor::vector_t& values) const;

  private:
    std::string  pattern_;
    std::regex   regex_;
    array_filter array_filter_;
};

// Subscription configuration for a client
struct subscription_config
{
    std::vector<std::string> include_patterns; // Paths to include
    std::vector<std::string> exclude_patterns; // Paths to exclude

    // Check if a path should be included in this subscription
    bool should_include(const std::string& path) const;

    // Apply array filtering to values based on subscription patterns
    caspar::core::monitor::vector_t apply_array_filters(const std::string&                     path,
                                                        const caspar::core::monitor::vector_t& values) const;

    // Check if subscription is empty (no includes)
    bool is_empty() const { return include_patterns.empty(); }
};

class websocket_monitor_client
{
  public:
    explicit websocket_monitor_client(std::shared_ptr<boost::asio::io_context> context);
    ~websocket_monitor_client();

    // Connection management with subscription support
    void add_connection(const std::string&                      connection_id,
                        std::function<void(const std::string&)> send_callback,
                        const subscription_config&              subscription = subscription_config{});

    void remove_connection(const std::string& connection_id);

    // Update subscription for existing connection
    void update_subscription(const std::string& connection_id, const subscription_config& subscription);

    // Send state to all connections (filtered by their subscriptions)
    void send(const caspar::core::monitor::state& state);

    // Force disconnect all (for shutdown)
    void force_disconnect_all();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::protocol::websocket