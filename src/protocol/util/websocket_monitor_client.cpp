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
 */

#include "../StdAfx.h"

#include "websocket_monitor_client.h"
#include "websocket_monitor_server.h"

#include <common/log.h>
#include <common/utf.h>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

#include <tbb/concurrent_hash_map.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace caspar { namespace protocol { namespace websocket {

using json = nlohmann::json;

namespace {

// Convert variant to JSON value
json variant_to_json_value(const core::monitor::data_t& value)
{
    if (auto bool_val = boost::get<bool>(&value)) {
        return *bool_val;
    } else if (auto int32_val = boost::get<std::int32_t>(&value)) {
        return *int32_val;
    } else if (auto int64_val = boost::get<std::int64_t>(&value)) {
        return *int64_val;
    } else if (auto uint32_val = boost::get<std::uint32_t>(&value)) {
        return *uint32_val;
    } else if (auto uint64_val = boost::get<std::uint64_t>(&value)) {
        return *uint64_val;
    } else if (auto float_val = boost::get<float>(&value)) {
        return *float_val;
    } else if (auto double_val = boost::get<double>(&value)) {
        return *double_val;
    } else if (auto string_val = boost::get<std::string>(&value)) {
        return *string_val;
    } else if (auto wstring_val = boost::get<std::wstring>(&value)) {
        return u8(*wstring_val);
    }
    return nullptr; // JSON null
}

// Convert OSC-style path to nested JSON structure
void set_nested_json_value(json& root, const std::string& path, const core::monitor::vector_t& values)
{
    std::vector<std::string> parts;
    boost::split(parts, path, boost::is_any_of("/"));

    // Remove empty parts (from leading slash)
    parts.erase(std::remove_if(parts.begin(), parts.end(), [](const std::string& s) { return s.empty(); }),
                parts.end());

    json* current = &root;

    // Navigate to the parent node, creating nodes as needed
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!current->contains(parts[i])) {
            (*current)[parts[i]] = json::object();
        }
        current = &(*current)[parts[i]];
    }

    // Set the value(s)
    if (!parts.empty()) {
        const std::string& key = parts.back();
        if (values.empty()) {
            (*current)[key] = nullptr; // JSON null
        } else if (values.size() == 1) {
            (*current)[key] = variant_to_json_value(values[0]);
        } else {
            json array = json::array();
            for (const auto& value : values) {
                array.push_back(variant_to_json_value(value));
            }
            (*current)[key] = array;
        }
    }
}

// Convert monitor state to OSC-style JSON structure
std::string monitor_state_to_osc_json(const core::monitor::state& state, const std::string& message_type)
{
    json root;

    // Add message metadata
    root["type"] = message_type;
    root["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Convert monitor state to nested JSON structure
    json data_node = json::object();
    for (const auto& [path, values] : state) {
        set_nested_json_value(data_node, path, values);
    }

    root["data"] = data_node;

    return root.dump();
}

// Generate delta changes between two monitor states
std::vector<std::pair<std::string, core::monitor::vector_t>> generate_deltas(const core::monitor::state& old_state,
                                                                             const core::monitor::state& new_state)
{
    std::vector<std::pair<std::string, core::monitor::vector_t>> deltas;

    // Find changed/new values
    for (const auto& [path, new_values] : new_state) {
        // Find the corresponding value in old_state using linear search
        auto old_it =
            std::find_if(old_state.begin(), old_state.end(), [&path](const auto& pair) { return pair.first == path; });

        if (old_it == old_state.end() || old_it->second != new_values) {
            deltas.emplace_back(path, new_values);
        }
    }

    // Find removed values (set to null/empty)
    for (const auto& [path, old_values] : old_state) {
        auto new_it =
            std::find_if(new_state.begin(), new_state.end(), [&path](const auto& pair) { return pair.first == path; });

        if (new_it == new_state.end()) {
            deltas.emplace_back(path, core::monitor::vector_t{}); // Empty vector indicates removal
        }
    }

    return deltas;
}

// Convert deltas to RFC 6902 JSON Patch format
std::string deltas_to_json_patch(const std::vector<std::pair<std::string, core::monitor::vector_t>>& deltas)
{
    json root;

    root["type"] = "patch";
    root["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Build the JSON Patch array
    json patch_array = json::array();

    for (const auto& [path, values] : deltas) {
        std::vector<std::string> parts;
        boost::split(parts, path, boost::is_any_of("/"));

        // Remove empty parts (from leading slash)
        parts.erase(std::remove_if(parts.begin(), parts.end(), [](const std::string& s) { return s.empty(); }),
                    parts.end());

        if (values.empty()) {
            // Remove operation
            json patch_op;
            patch_op["op"]   = "remove";
            patch_op["path"] = "/" + boost::join(parts, "/");
            patch_array.push_back(patch_op);
        } else {
            // Add/replace operation
            json patch_op;

            // Determine if this is an add or replace operation
            // For simplicity, we'll use "replace" for existing paths and "add" for new ones
            // In practice, you might want to track which paths existed in the previous state
            patch_op["op"] = "replace"; // or "add" depending on your logic

            // Build the path
            patch_op["path"] = "/" + boost::join(parts, "/");

            // Set the value
            if (values.size() == 1) {
                patch_op["value"] = variant_to_json_value(values[0]);
            } else {
                json array = json::array();
                for (const auto& value : values) {
                    array.push_back(variant_to_json_value(value));
                }
                patch_op["value"] = array;
            }

            patch_array.push_back(patch_op);
        }
    }

    root["data"] = patch_array;

    return root.dump();
}

// Generate heartbeat message
std::string generate_heartbeat()
{
    json root;
    root["type"] = "heartbeat";
    root["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    return root.dump();
}

} // namespace

struct connection_state
{
    std::string                             connection_id;
    std::function<void(const std::string&)> send_callback;
    connection_config                       config;
    core::monitor::state                    last_state;
    std::chrono::steady_clock::time_point   last_full_state_time;
    std::chrono::steady_clock::time_point   last_heartbeat_time;
    bool                                    needs_full_state;

    connection_state(const std::string&                      id,
                     std::function<void(const std::string&)> callback,
                     const connection_config&                cfg)
        : connection_id(id)
        , send_callback(std::move(callback))
        , config(cfg)
        , last_full_state_time(std::chrono::steady_clock::now())
        , last_heartbeat_time(std::chrono::steady_clock::now())
        , needs_full_state(true)
    {
    }
};

struct websocket_monitor_client::impl
{
    std::shared_ptr<boost::asio::io_service>                           service_;
    std::mutex                                                         connections_mutex_;
    std::unordered_map<std::string, std::unique_ptr<connection_state>> connections_;

    // Subscription management
    tbb::concurrent_hash_map<std::string, std::weak_ptr<void>> subscriptions_;

    // Timer for periodic tasks
    std::unique_ptr<boost::asio::deadline_timer> timer_;
    std::atomic<bool>                            running_{true};

    explicit impl(std::shared_ptr<boost::asio::io_service> service)
        : service_(std::move(service))
        , timer_(std::make_unique<boost::asio::deadline_timer>(*service_))
    {
        schedule_periodic_tasks();
    }

    ~impl()
    {
        // Stop the periodic timer first
        running_ = false;
        if (timer_) {
            timer_->cancel();
        }

        // Clear all connections to ensure no callbacks are invoked
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
        subscriptions_.clear();
    }

    void schedule_periodic_tasks()
    {
        if (!running_)
            return;

        timer_->expires_from_now(boost::posix_time::seconds(1));
        timer_->async_wait([this](const boost::system::error_code& ec) {
            if (!ec && running_) {
                handle_periodic_tasks();
                // Only schedule next task if still running
                if (running_) {
                    schedule_periodic_tasks();
                }
            }
        });
    }

    void handle_periodic_tasks()
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto                        now = std::chrono::steady_clock::now();

        for (auto& [id, conn] : connections_) {
            try {
                // Check if we need to send full state (only if interval > 0)
                if (conn->config.full_state_interval.count() > 0 &&
                    std::chrono::duration_cast<std::chrono::seconds>(now - conn->last_full_state_time) >=
                        conn->config.full_state_interval) {
                    conn->needs_full_state = true;
                }

                // Check if we need to send heartbeat
                if (conn->config.send_heartbeat &&
                    std::chrono::duration_cast<std::chrono::seconds>(now - conn->last_heartbeat_time) >=
                        conn->config.heartbeat_interval) {
                    std::string heartbeat = generate_heartbeat();
                    conn->send_callback(heartbeat);
                    conn->last_heartbeat_time = now;
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(info) << L"WebSocket monitor: Removing failed connection " << u16(id) << L": "
                                 << u16(e.what());
                // Connection will be removed in next iteration
            }
        }
    }

    void send(const core::monitor::state& state)
    {
        if (connections_.empty()) {
            return;
        }

        // Prepare messages outside of lock to minimize lock time
        std::string full_state_json;
        bool        need_full_state = false;

        // Check if any connection needs full state
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (const auto& [id, conn] : connections_) {
                if (conn->needs_full_state) {
                    need_full_state = true;
                    break;
                }
            }
        }

        // Generate full state message if needed
        if (need_full_state) {
            full_state_json = monitor_state_to_osc_json(state, "full_state");
        }

        // Send to all connected clients with minimal lock time
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto                        it = connections_.begin();
        while (it != connections_.end()) {
            try {
                auto& conn = it->second;

                if (conn->needs_full_state) {
                    // Send full state
                    conn->send_callback(full_state_json);
                    conn->last_state           = state;
                    conn->last_full_state_time = std::chrono::steady_clock::now();
                    conn->needs_full_state     = false;
                } else {
                    // Send delta
                    auto deltas = generate_deltas(conn->last_state, state);
                    if (!deltas.empty()) {
                        std::string delta_json = deltas_to_json_patch(deltas);
                        conn->send_callback(delta_json);
                        conn->last_state = state;
                    }
                }

                ++it;
            } catch (const std::exception& e) {
                CASPAR_LOG(info) << L"WebSocket monitor: Removing failed connection " << u16(it->first) << L": "
                                 << u16(e.what());
                it = connections_.erase(it);
            }
        }
    }

    void add_connection(const std::string&                      connection_id,
                        std::function<void(const std::string&)> send_callback,
                        const connection_config&                config)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connection_id] =
            std::make_unique<connection_state>(connection_id, std::move(send_callback), config);
        CASPAR_LOG(info) << L"WebSocket monitor: Added connection " << u16(connection_id) << L" ("
                         << connections_.size() << L" total connections)";
    }

    void remove_connection(const std::string& connection_id)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connection_id);
        CASPAR_LOG(info) << L"WebSocket monitor: Removed connection " << u16(connection_id) << L" ("
                         << connections_.size() << L" total connections)";
    }

    void force_full_state()
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [id, conn] : connections_) {
            conn->needs_full_state = true;
        }
        CASPAR_LOG(info) << L"WebSocket monitor: Forced full state for all connections";
    }

    void request_full_state(const std::string& connection_id)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto                        it = connections_.find(connection_id);
        if (it != connections_.end()) {
            it->second->needs_full_state = true;
            CASPAR_LOG(info) << L"WebSocket monitor: Requested full state for connection " << u16(connection_id);
        } else {
            CASPAR_LOG(warning) << L"WebSocket monitor: Connection not found for full state request: "
                                << u16(connection_id);
        }
    }

    void force_disconnect_all();

    std::shared_ptr<void> get_subscription_token(const std::string& connection_id)
    {
        // Create a token that when destroyed will clean up the subscription
        auto token = std::shared_ptr<void>(nullptr, [this, connection_id](void*) {
            // Remove subscription when token is destroyed
            subscriptions_.erase(connection_id);
        });

        // Store weak reference to token
        subscriptions_.insert(std::make_pair(connection_id, token));

        return token;
    }
};

void caspar::protocol::websocket::websocket_monitor_client::impl::force_disconnect_all()
{
    // Stop the periodic timer first to prevent new tasks
    running_ = false;
    if (timer_) {
        timer_->cancel();
    }

    // Clear all connections and subscriptions
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto                        connection_count = connections_.size();
    connections_.clear();
    subscriptions_.clear();

    CASPAR_LOG(info) << L"WebSocket monitor: Force disconnected " << connection_count << L" connections";
}

websocket_monitor_client::websocket_monitor_client(std::shared_ptr<boost::asio::io_service> service)
    : impl_(spl::make_shared<impl>(std::move(service)))
{
}

websocket_monitor_client::websocket_monitor_client(websocket_monitor_client&& other)
    : impl_(std::move(other.impl_))
{
}

websocket_monitor_client& websocket_monitor_client::operator=(websocket_monitor_client&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}

websocket_monitor_client::~websocket_monitor_client() = default;

std::shared_ptr<void> websocket_monitor_client::get_subscription_token(const std::string& connection_id)
{
    return impl_->get_subscription_token(connection_id);
}

void websocket_monitor_client::send(const core::monitor::state& state) { impl_->send(state); }

void websocket_monitor_client::add_connection(const std::string&                      connection_id,
                                              std::function<void(const std::string&)> send_callback,
                                              const connection_config&                config)
{
    impl_->add_connection(connection_id, std::move(send_callback), config);
}

void websocket_monitor_client::remove_connection(const std::string& connection_id)
{
    impl_->remove_connection(connection_id);
}

void websocket_monitor_client::force_full_state() { impl_->force_full_state(); }

void websocket_monitor_client::request_full_state(const std::string& connection_id)
{
    impl_->request_full_state(connection_id);
}

void websocket_monitor_client::force_disconnect_all() { impl_->force_disconnect_all(); }

}}} // namespace caspar::protocol::websocket