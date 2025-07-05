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

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    // Build hash maps for O(1) lookups
    std::unordered_map<std::string, core::monitor::vector_t> old_state_map;
    std::unordered_map<std::string, core::monitor::vector_t> new_state_map;

    for (const auto& [path, values] : old_state) {
        old_state_map[path] = values;
    }

    for (const auto& [path, values] : new_state) {
        new_state_map[path] = values;
    }

    // Find changed/new values
    for (const auto& [path, new_values] : new_state_map) {
        auto old_it = old_state_map.find(path);
        if (old_it == old_state_map.end() || old_it->second != new_values) {
            deltas.emplace_back(path, new_values);
        }
    }

    // Find removed values (set to null/empty)
    for (const auto& [path, old_values] : old_state_map) {
        if (new_state_map.find(path) == new_state_map.end()) {
            deltas.emplace_back(path, core::monitor::vector_t{}); // Empty vector indicates removal
        }
    }

    return deltas;
}

// Convert deltas to RFC 7386 JSON Merge Patch format
std::string deltas_to_json_merge_patch(const std::vector<std::pair<std::string, core::monitor::vector_t>>& deltas,
                                       const std::unordered_set<std::string>& existing_prefixes)
{
    json root;

    root["type"] = "merge_patch";
    root["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Build the JSON Merge Patch object
    json patch_obj = json::object();

    // Helper to apply delta into patch_obj, creating intermediate objects as needed.
    auto apply_delta_to_json = [&](const std::string& path, const core::monitor::vector_t& vals) {
        // Parse path segments more efficiently
        std::vector<std::string> parts;
        parts.reserve(8); // Most paths are shallow

        size_t start = 0;
        size_t pos   = 0;
        while ((pos = path.find('/', start)) != std::string::npos) {
            if (pos > start) {
                parts.push_back(path.substr(start, pos - start));
            }
            start = pos + 1;
        }
        if (start < path.length()) {
            parts.push_back(path.substr(start));
        }

        json* current = &patch_obj;
        for (size_t i = 0; i < parts.size(); ++i) {
            const std::string& key = parts[i];

            if (i + 1 == parts.size()) {
                // Leaf node
                if (vals.empty()) {
                    // Removal represented by null in merge patch
                    (*current)[key] = nullptr;
                } else if (vals.size() == 1) {
                    (*current)[key] = variant_to_json_value(vals[0]);
                } else {
                    json array = json::array();
                    for (const auto& v : vals) {
                        array.push_back(variant_to_json_value(v));
                    }
                    (*current)[key] = array;
                }
            } else {
                // Intermediate node - ensure object exists
                if (!current->contains(key) || !(*current)[key].is_object()) {
                    (*current)[key] = json::object();
                }
                current = &(*current)[key];
            }
        }
    };

    // Separate removal and update deltas so we can minimise removal paths
    std::vector<std::string>                                     removal_paths;
    std::vector<std::pair<std::string, core::monitor::vector_t>> update_deltas;

    for (const auto& [path, values] : deltas) {
        if (values.empty()) {
            removal_paths.push_back(path);
        } else {
            update_deltas.emplace_back(path, values);
        }
    }

    // Sort removal paths by length (shortest first) so that parents appear before children
    std::sort(removal_paths.begin(), removal_paths.end(), [](const std::string& a, const std::string& b) {
        return a.size() < b.size();
    });

    // Build a minimal set of removal paths (skip children of an already-removed parent and climb up until no updates
    // exist under the same branch)
    std::unordered_set<std::string> selected_removals;
    for (std::string path : removal_paths) {
        // Ascend until path would overlap with an update or until root
        while (true) {
            auto pos = path.find_last_of('/');
            if (pos == std::string::npos) {
                break;
            }
            std::string parent = path.substr(0, pos);

            // If parent still exists in the new_state, we must stop here
            if (existing_prefixes.count(parent) > 0) {
                break;
            }

            // Otherwise we can move up one level
            path = parent;
        }

        // Check if an ancestor is already selected to avoid duplicates
        bool        covered  = false;
        std::string ancestor = path;
        while (true) {
            auto pos = ancestor.find_last_of('/');
            if (pos == std::string::npos) {
                break;
            }
            ancestor = ancestor.substr(0, pos);
            if (selected_removals.count(ancestor)) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            selected_removals.insert(path);
        }
    }

    // Apply removals (set to nullptr)
    for (const auto& path : selected_removals) {
        apply_delta_to_json(path, core::monitor::vector_t{});
    }

    // Apply updates
    for (const auto& [path, values] : update_deltas) {
        apply_delta_to_json(path, values);
    }

    root["data"] = patch_obj;

    return root.dump();
}

// Pre-compute all path prefixes from monitor state
std::unordered_set<std::string> build_existing_prefixes(const core::monitor::state& state)
{
    std::unordered_set<std::string> prefixes;
    prefixes.reserve(std::distance(state.begin(), state.end()) * 4); // Estimate: most paths have ~4 segments

    for (const auto& [path, _] : state) {
        std::string prefix;
        prefix.reserve(path.length());

        size_t start = 0;
        size_t pos   = 0;
        while ((pos = path.find('/', start)) != std::string::npos) {
            if (pos > start) {
                if (!prefix.empty()) {
                    prefix += "/";
                }
                prefix += path.substr(start, pos - start);
                prefixes.insert(prefix);
            }
            start = pos + 1;
        }
        if (start < path.length()) {
            if (!prefix.empty()) {
                prefix += "/";
            }
            prefix += path.substr(start);
            prefixes.insert(prefix);
        }
    }

    return prefixes;
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

        // Pre-compute existing prefixes once for all connections
        auto existing_prefixes = build_existing_prefixes(state);

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
                        std::string delta_json = deltas_to_json_merge_patch(deltas, existing_prefixes);
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