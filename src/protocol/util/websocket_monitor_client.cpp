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

#include <common/log.h>
#include <common/utf.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace caspar { namespace protocol { namespace websocket {

using json = nlohmann::json;

// Simple JSON conversion functions
json variant_to_json_value(const core::monitor::data_t& value)
{
    // nlohmann/json can handle most types automatically
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

void set_nested_json_value(json& root, const std::string& path, const core::monitor::vector_t& values)
{
    std::list<std::string> path_parts;
    boost::split(path_parts, path, boost::is_any_of("/"));

    json* current = &root;
    for (auto it = path_parts.begin(); it != path_parts.end(); ++it) {
        if (it->empty())
            continue;

        if (std::next(it) == path_parts.end()) {
            // Leaf node - set the value
            if (values.size() == 1) {
                (*current)[*it] = variant_to_json_value(values[0]);
            } else {
                json array = json::array();
                for (const auto& value : values) {
                    array.push_back(variant_to_json_value(value));
                }
                (*current)[*it] = array;
            }
        } else {
            // Intermediate node - create object if needed
            if (!current->contains(*it)) {
                (*current)[*it] = json::object();
            }
            current = &(*current)[*it];
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

// Simplified connection tracking with state for deltas
struct connection_info
{
    std::string                             connection_id;
    std::function<void(const std::string&)> send_callback;
    core::monitor::state                    last_state;
    bool                                    needs_full_state;

    connection_info(std::string id, std::function<void(const std::string&)> callback)
        : connection_id(std::move(id))
        , send_callback(std::move(callback))
        , needs_full_state(true)
    {
    }
};

struct websocket_monitor_client::impl
{
    std::shared_ptr<boost::asio::io_context>                          context_;
    std::mutex                                                        connections_mutex_;
    std::unordered_map<std::string, std::unique_ptr<connection_info>> connections_;

    explicit impl(std::shared_ptr<boost::asio::io_context> context)
        : context_(std::move(context))
    {
    }

    void add_connection(const std::string& connection_id, std::function<void(const std::string&)> send_callback)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connection_id] = std::make_unique<connection_info>(connection_id, std::move(send_callback));
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

    void send(const core::monitor::state& state)
    {
        if (connections_.empty()) {
            return;
        }

        // Send to all connections
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto                        it = connections_.begin();
        while (it != connections_.end()) {
            try {
                auto& conn = it->second;

                if (conn->needs_full_state) {
                    // Send full state
                    std::string full_state_json = monitor_state_to_osc_json(state, "full_state");
                    conn->send_callback(full_state_json);
                    conn->last_state       = state;
                    conn->needs_full_state = false;
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

    void force_disconnect_all()
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
        CASPAR_LOG(info) << L"WebSocket monitor: Force disconnected all connections";
    }

    void force_full_state()
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [id, conn] : connections_) {
            conn->needs_full_state = true;
        }
        CASPAR_LOG(info) << L"WebSocket monitor: Forced full state for all connections";
    }
};

websocket_monitor_client::websocket_monitor_client(std::shared_ptr<boost::asio::io_context> context)
    : impl_(std::make_unique<impl>(std::move(context)))
{
}

websocket_monitor_client::~websocket_monitor_client()
{
    // impl_ will be automatically destroyed
}

void websocket_monitor_client::add_connection(const std::string&                      connection_id,
                                              std::function<void(const std::string&)> send_callback)
{
    impl_->add_connection(connection_id, std::move(send_callback));
}

void websocket_monitor_client::remove_connection(const std::string& connection_id)
{
    impl_->remove_connection(connection_id);
}

void websocket_monitor_client::send(const core::monitor::state& state) { impl_->send(state); }

void websocket_monitor_client::force_disconnect_all() { impl_->force_disconnect_all(); }

void websocket_monitor_client::force_full_state() { impl_->force_full_state(); }

}}} // namespace caspar::protocol::websocket