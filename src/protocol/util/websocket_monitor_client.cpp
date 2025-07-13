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

#include "../StdAfx.h"

#include "websocket_monitor_client.h"

#include <common/log.h>
#include <common/utf.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <chrono>
#include <core/monitor/monitor.h>
#include <list>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace caspar { namespace protocol { namespace websocket {

// Parse array filter from pattern like "volume[0:2]" or "volume[0]"
std::pair<std::string, array_filter> parse_array_pattern(const std::string& pattern)
{
    array_filter filter;

    // Find the last '[' that could be an array index
    size_t bracket_pos = pattern.find_last_of('[');
    if (bracket_pos == std::string::npos) {
        return {pattern, filter}; // No array indexing
    }

    // Check if this looks like array indexing (not character class)
    // Array indexing: [0], [0:2], [*]
    // Character class: [a-z], [0-9], [abc]
    size_t close_bracket = pattern.find(']', bracket_pos);
    if (close_bracket == std::string::npos) {
        return {pattern, filter}; // No closing bracket
    }

    std::string bracket_content = pattern.substr(bracket_pos + 1, close_bracket - bracket_pos - 1);

    // Check if this is array indexing (has colon) vs character class/number range
    // Array indexing: [0:2], [*], [5] (single digit only)
    // Character/Number ranges: [1-9], [1-20], [a-z], [abc]
    bool is_array_indexing = false;

    if (bracket_content == "*") {
        is_array_indexing = true;
    } else if (bracket_content.find(':') != std::string::npos) {
        is_array_indexing = true;
    } else if (std::all_of(bracket_content.begin(), bracket_content.end(), ::isdigit) &&
               bracket_content.length() == 1) {
        // Single digit like [0] is array indexing
        is_array_indexing = true;
    }

    if (!is_array_indexing) {
        return {pattern, filter}; // This is a character class or number range, not array indexing
    }

    // Parse the array indexing
    filter.has_filter = true;

    if (bracket_content == "*") {
        // [*] means all elements - no actual filtering needed
        filter.has_filter = false;
    } else if (bracket_content.find(':') != std::string::npos) {
        // Range like [0:2]
        size_t      colon_pos = bracket_content.find(':');
        std::string start_str = bracket_content.substr(0, colon_pos);
        std::string end_str   = bracket_content.substr(colon_pos + 1);

        try {
            filter.start        = std::stoi(start_str);
            filter.end          = std::stoi(end_str);
            filter.single_index = false;
        } catch (const std::exception&) {
            filter.has_filter = false; // Invalid format
        }
    } else {
        // Single index like [0]
        try {
            filter.start        = std::stoi(bracket_content);
            filter.single_index = true;
        } catch (const std::exception&) {
            filter.has_filter = false; // Invalid format
        }
    }

    // Return the path without the array indexing part
    std::string base_path = pattern.substr(0, bracket_pos);
    return {base_path, filter};
}

// Path matcher implementation
path_matcher::path_matcher(const std::string& pattern)
    : pattern_(pattern)
{
    // Parse array indexing from pattern
    auto [base_pattern, array_filter] = parse_array_pattern(pattern);
    array_filter_                     = array_filter;

    // Convert POSIX glob pattern to regex
    std::string regex_pattern;
    regex_pattern.reserve(base_pattern.length() * 2); // Reserve space for efficiency

    for (size_t i = 0; i < base_pattern.length(); ++i) {
        char c = base_pattern[i];

        switch (c) {
            case '*':
                // * matches any sequence of characters
                regex_pattern += ".*";
                break;

            case '[':
                // Character class or number range
                // Handle [a-z], [0-9], [1-20], [abc], [!abc] etc.
                {
                    size_t bracket_start = i;
                    ++i;

                    // Find the closing bracket
                    size_t bracket_end = base_pattern.find(']', i);
                    if (bracket_end == std::string::npos) {
                        // No closing bracket, treat as literal
                        regex_pattern += "\\[";
                        --i; // Back up to process the '[' as literal next time
                        break;
                    }

                    std::string bracket_content = base_pattern.substr(i, bracket_end - i);
                    bool        is_negated      = false;

                    // Handle negation [!...] -> [^...]
                    if (!bracket_content.empty() && bracket_content[0] == '!') {
                        is_negated      = true;
                        bracket_content = bracket_content.substr(1);
                    }

                    // Check if this is a number range like [1-20] or number alternation like [90|100|150]
                    std::regex  number_range_regex(R"(^(\d+)-(\d+)$)");
                    std::regex  number_alternation_regex(R"(^(\d+)(\|\d+)+$)");
                    std::smatch match;

                    if (std::regex_match(bracket_content, match, number_range_regex)) {
                        // This is a number range like [1-20]
                        int start_num = std::stoi(match[1].str());
                        int end_num   = std::stoi(match[2].str());

                        if (start_num <= end_num) {
                            // Generate alternation: (1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20)
                            regex_pattern += "(";
                            if (is_negated) {
                                // For negated number ranges, we need to use negative lookahead
                                regex_pattern += "(?!";
                                for (int num = start_num; num <= end_num; ++num) {
                                    if (num > start_num)
                                        regex_pattern += "|";
                                    regex_pattern += std::to_string(num);
                                }
                                regex_pattern += ")\\d+"; // Match any number that's not in the range
                            } else {
                                for (int num = start_num; num <= end_num; ++num) {
                                    if (num > start_num)
                                        regex_pattern += "|";
                                    regex_pattern += std::to_string(num);
                                }
                            }
                            regex_pattern += ")";
                        } else {
                            // Invalid range, treat as literal
                            regex_pattern +=
                                "\\[" + base_pattern.substr(bracket_start + 1, bracket_end - bracket_start) + "\\]";
                        }
                    } else if (std::regex_match(bracket_content, number_alternation_regex)) {
                        // This is number alternation like [90|100|150]
                        regex_pattern += "(";
                        if (is_negated) {
                            // For negated number alternation, use negative lookahead
                            regex_pattern += "(?!";
                            // Split by | and add each number
                            std::string temp_content = bracket_content;
                            std::replace(temp_content.begin(), temp_content.end(), '|', ' ');
                            std::istringstream iss(temp_content);
                            std::string        number;
                            bool               first = true;
                            while (iss >> number) {
                                if (!first)
                                    regex_pattern += "|";
                                regex_pattern += number;
                                first = false;
                            }
                            regex_pattern += ")\\d+"; // Match any number that's not in the list
                        } else {
                            // Replace | with | for regex alternation
                            regex_pattern += bracket_content;
                        }
                        regex_pattern += ")";
                    } else {
                        // Regular character class - pass through to regex
                        regex_pattern += '[';
                        if (is_negated) {
                            regex_pattern += '^';
                        }

                        // Copy the character class content
                        for (size_t j = 0; j < bracket_content.length(); ++j) {
                            if (bracket_content[j] == '\\' && j + 1 < bracket_content.length()) {
                                // Handle escaped characters within character class
                                regex_pattern += '\\';
                                regex_pattern += bracket_content[++j];
                            } else {
                                regex_pattern += bracket_content[j];
                            }
                        }
                        regex_pattern += ']';
                    }

                    i = bracket_end; // Skip to after the closing bracket
                }
                break;

            case '\\':
                // Escape character - pass through the escaped character
                if (i + 1 < base_pattern.length()) {
                    regex_pattern += '\\';
                    regex_pattern += base_pattern[++i];
                } else {
                    regex_pattern += "\\\\"; // Trailing backslash
                }
                break;

            // Escape regex special characters that aren't glob wildcards
            case '^':
            case '$':
            case '.':
            case '(':
            case ')':
            case '{':
            case '}':
            case '+':
            case '?':
            case '|':
                regex_pattern += '\\';
                regex_pattern += c;
                break;

            default:
                // Regular character
                regex_pattern += c;
                break;
        }
    }

    // Anchor the pattern to match the entire string
    regex_pattern = "^" + regex_pattern + "$";

    try {
        regex_ = std::regex(regex_pattern, std::regex_constants::icase);
    } catch (const std::regex_error&) {
        // If regex compilation fails, create a pattern that matches nothing
        regex_ = std::regex("(?!)");
    }
}

bool path_matcher::matches(const std::string& path) const { return std::regex_match(path, regex_); }

bool path_matcher::has_array_filter() const { return array_filter_.has_filter; }

caspar::core::monitor::vector_t path_matcher::apply_array_filter(const caspar::core::monitor::vector_t& values) const
{
    if (!array_filter_.has_filter) {
        return values; // No filtering, return all values
    }

    caspar::core::monitor::vector_t filtered_values;

    if (array_filter_.single_index) {
        // Single index like [0]
        if (array_filter_.start >= 0 && array_filter_.start < static_cast<int>(values.size())) {
            filtered_values.push_back(values[array_filter_.start]);
        }
    } else {
        // Range like [0:2]
        int start = (array_filter_.start < 0) ? 0 : array_filter_.start;
        int end   = array_filter_.end;
        if (end < 0 || end > static_cast<int>(values.size())) {
            end = static_cast<int>(values.size());
        }

        for (int i = start; i < end && i < static_cast<int>(values.size()); ++i) {
            filtered_values.push_back(values[i]);
        }
    }

    return filtered_values;
}

// Subscription configuration implementation
bool subscription_config::should_include(const std::string& path) const
{
    // If no include patterns specified, include nothing by default
    // This ensures clients must explicitly subscribe to receive data
    if (include_patterns.empty()) {
        return false; // No subscription = no data
    }

    // Check if path matches any include pattern
    bool included = false;
    for (const auto& include_pattern : include_patterns) {
        path_matcher matcher(include_pattern);
        if (matcher.matches(path)) {
            included = true;
            break;
        }
    }

    if (!included) {
        return false; // Not included
    }

    // Check exclude patterns (exclude takes precedence)
    for (const auto& exclude_pattern : exclude_patterns) {
        path_matcher matcher(exclude_pattern);
        if (matcher.matches(path)) {
            return false; // Excluded
        }
    }

    return true; // Included and not excluded
}

// Apply array filtering to monitor data based on subscription patterns
caspar::core::monitor::vector_t
subscription_config::apply_array_filters(const std::string& path, const caspar::core::monitor::vector_t& values) const
{
    // Check all include patterns for array filters
    for (const auto& include_pattern : include_patterns) {
        path_matcher matcher(include_pattern);
        if (matcher.matches(path) && matcher.has_array_filter()) {
            // Apply the array filter
            return matcher.apply_array_filter(values);
        }
    }

    // No array filtering applied, return original values
    return values;
}

// Simple JSON conversion functions
json variant_to_json_value(const caspar::core::monitor::data_t& value)
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

void set_nested_json_value(json& root, const std::string& path, const caspar::core::monitor::vector_t& values)
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
std::string monitor_state_to_osc_json(const caspar::core::monitor::state& state, const std::string& message_type)
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

// Extract specific layer numbers from subscription patterns
std::set<int> extract_subscribed_layer_numbers(const std::vector<std::string>& patterns)
{
    std::set<int> layer_numbers;

    for (const auto& pattern : patterns) {
        // Look for patterns like "channel/*/stage/layer/123/*" or "channel/1/stage/layer/123/*"
        // Also handle patterns like "channel/1/stage/layer/[1-9]/*" and "channel/1/stage/layer/[90|100|150]/*"

        // Split the pattern by '/'
        std::vector<std::string> parts;
        boost::split(parts, pattern, boost::is_any_of("/"));

        // Look for the pattern: [...]/stage/layer/{number_or_pattern}/[...]
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i >= 2 && parts[i - 2] == "stage" && parts[i - 1] == "layer" && parts[i] != "*" && !parts[i].empty()) {
                if (parts[i].find('[') == std::string::npos && parts[i].find('*') == std::string::npos) {
                    // Simple specific number like "100"
                    try {
                        int layer_num = std::stoi(parts[i]);
                        layer_numbers.insert(layer_num);
                    } catch (const std::exception&) {
                        // Not a valid number, skip
                    }
                } else if (parts[i].front() == '[' && parts[i].back() == ']') {
                    // Pattern like [1-9] or [90|100|150]
                    std::string bracket_content = parts[i].substr(1, parts[i].length() - 2);

                    // Check if this is a number range like [1-9] or [1-20]
                    std::regex  number_range_regex(R"(^(\d+)-(\d+)$)");
                    std::smatch match;

                    if (std::regex_match(bracket_content, match, number_range_regex)) {
                        // Number range like [1-9] or [1-20]
                        int start_num = std::stoi(match[1].str());
                        int end_num   = std::stoi(match[2].str());

                        if (start_num <= end_num) {
                            for (int num = start_num; num <= end_num; ++num) {
                                layer_numbers.insert(num);
                            }
                        }
                    } else {
                        // Check if this is number alternation like [90|100|150]
                        std::regex number_alternation_regex(R"(^(\d+)(\|\d+)+$)");

                        if (std::regex_match(bracket_content, number_alternation_regex)) {
                            // Split by | and extract each number
                            std::vector<std::string> alt_parts;
                            boost::split(alt_parts, bracket_content, boost::is_any_of("|"));

                            for (const auto& alt_part : alt_parts) {
                                try {
                                    int layer_num = std::stoi(alt_part);
                                    layer_numbers.insert(layer_num);
                                } catch (const std::exception&) {
                                    // Not a valid number, skip
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return layer_numbers;
}

// Check if a layer number exists in the monitor state
bool layer_exists_in_state(const caspar::core::monitor::state& state, int layer_number)
{
    std::string layer_prefix = "channel/";
    std::string layer_suffix = "/stage/layer/" + std::to_string(layer_number);

    for (const auto& [path, values] : state) {
        if (path.find(layer_suffix) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Create empty layer data for a specific layer
void inject_empty_layer_data(caspar::core::monitor::state& state, int channel_id, int layer_number)
{
    std::string layer_prefix = "channel/" + std::to_string(channel_id) + "/stage/layer/" + std::to_string(layer_number);

    // Add empty foreground producer data
    state[layer_prefix + "/foreground/producer"] = {std::string("empty")};
    state[layer_prefix + "/foreground/paused"]   = {false};
    state[layer_prefix + "/foreground/uid"]      = {std::wstring(L"")};

    // Add empty background producer data
    state[layer_prefix + "/background/producer"] = {std::string("empty")};
    state[layer_prefix + "/background/uid"]      = {std::wstring(L"")};
}

// Pre-compute all path prefixes from monitor state (for future path-based subscriptions)
std::unordered_set<std::string> build_existing_prefixes(const caspar::core::monitor::state& state)
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

// Connection tracking with subscription support
struct connection_info
{
    std::string                             connection_id;
    std::function<void(const std::string&)> send_callback;
    subscription_config                     subscription;
    caspar::core::monitor::state            last_state; // Keep for future path-based subscriptions

    connection_info(std::string id, std::function<void(const std::string&)> callback, subscription_config sub)
        : connection_id(std::move(id))
        , send_callback(std::move(callback))
        , subscription(std::move(sub))
    {
    }
};

struct websocket_monitor_client::impl
{
    std::shared_ptr<boost::asio::io_context>                          context_;
    std::mutex                                                        connections_mutex_;
    std::unordered_map<std::string, std::unique_ptr<connection_info>> connections_;
    caspar::core::monitor::state current_state_; // Store current state for full state requests

    // Dedicated thread pool for WebSocket processing to avoid blocking the main IO context
    std::shared_ptr<boost::asio::thread_pool> worker_pool_;

    explicit impl(std::shared_ptr<boost::asio::io_context> context)
        : context_(std::move(context))
        , worker_pool_(std::make_shared<boost::asio::thread_pool>(2)) // 2 worker threads
    {
    }

    void add_connection(const std::string&                      connection_id,
                        std::function<void(const std::string&)> send_callback,
                        const subscription_config&              subscription)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connection_id] =
            std::make_unique<connection_info>(connection_id, std::move(send_callback), subscription);
        CASPAR_LOG(info) << L"WebSocket monitor: Added connection " << u16(connection_id) << L" ("
                         << connections_.size() << L" total connections) with subscription";
    }

    void remove_connection(const std::string& connection_id)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connection_id);
        CASPAR_LOG(info) << L"WebSocket monitor: Removed connection " << u16(connection_id) << L" ("
                         << connections_.size() << L" total connections)";
    }

    void update_subscription(const std::string& connection_id, const subscription_config& subscription)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto                        it = connections_.find(connection_id);
        if (it != connections_.end()) {
            it->second->subscription = subscription;
            CASPAR_LOG(info) << L"WebSocket monitor: Updated subscription for connection " << u16(connection_id);
        } else {
            CASPAR_LOG(warning) << L"WebSocket monitor: Connection not found for subscription update: "
                                << u16(connection_id);
        }
    }

    void send(const caspar::core::monitor::state& state)
    {
        // CRITICAL: Never block the channel thread!
        // Post all WebSocket work to dedicated thread pool to avoid IO context conflicts
        auto state_copy = std::make_shared<caspar::core::monitor::state>(state);

        boost::asio::post(*worker_pool_, [this, state_copy]() {
            try {
                // Store current state for full state requests (on worker thread)
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    current_state_ = *state_copy;
                }

                if (connections_.empty()) {
                    return;
                }

                // Pre-compute common data to avoid redundant work
                std::unordered_map<std::string, std::set<int>> connection_subscribed_layers;
                std::unordered_map<std::string, std::set<int>> connection_channel_ids;

                // First pass: collect subscription data for all connections
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    for (const auto& [conn_id, conn] : connections_) {
                        // Extract subscribed layers for this connection
                        auto subscribed_layers = extract_subscribed_layer_numbers(conn->subscription.include_patterns);
                        if (!subscribed_layers.empty()) {
                            connection_subscribed_layers[conn_id] = std::move(subscribed_layers);
                        }

                        // Extract channel IDs for this connection
                        std::set<int> channel_ids;
                        for (const auto& pattern : conn->subscription.include_patterns) {
                            std::vector<std::string> parts;
                            boost::split(parts, pattern, boost::is_any_of("/"));

                            if (parts.size() >= 2 && parts[0] == "channel") {
                                if (parts[1] != "*" && parts[1].find('[') == std::string::npos) {
                                    try {
                                        int channel_id = std::stoi(parts[1]);
                                        channel_ids.insert(channel_id);
                                    } catch (const std::exception&) {
                                        // Not a valid number, skip
                                    }
                                } else {
                                    // Wildcard or pattern for channel, use all existing channels from state
                                    for (const auto& [path, values] : *state_copy) {
                                        if (path.find("channel/") == 0) {
                                            size_t start = 8; // After "channel/"
                                            size_t end   = path.find('/', start);
                                            if (end != std::string::npos) {
                                                try {
                                                    int channel_id = std::stoi(path.substr(start, end - start));
                                                    channel_ids.insert(channel_id);
                                                } catch (const std::exception&) {
                                                    // Not a valid number, skip
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (!channel_ids.empty()) {
                            connection_channel_ids[conn_id] = std::move(channel_ids);
                        }
                    }
                }

                // Second pass: process each connection efficiently
                std::lock_guard<std::mutex> lock(connections_mutex_);
                auto                        it = connections_.begin();
                while (it != connections_.end()) {
                    try {
                        auto&              conn    = it->second;
                        const std::string& conn_id = it->first;

                        // Start with the original state (no expensive copying)
                        const caspar::core::monitor::state& base_state = *state_copy;

                        // Check if this connection needs layer injection
                        auto layers_it = connection_subscribed_layers.find(conn_id);
                        if (layers_it != connection_subscribed_layers.end()) {
                            // Create enhanced state only if needed
                            caspar::core::monitor::state enhanced_state = base_state;

                            auto channels_it = connection_channel_ids.find(conn_id);
                            if (channels_it != connection_channel_ids.end()) {
                                // Inject empty layer data for subscribed layers that don't exist
                                for (int layer_number : layers_it->second) {
                                    if (!layer_exists_in_state(enhanced_state, layer_number)) {
                                        for (int channel_id : channels_it->second) {
                                            inject_empty_layer_data(enhanced_state, channel_id, layer_number);
                                        }
                                    }
                                }

                                // Use enhanced state for filtering
                                caspar::core::monitor::state filtered_state;
                                for (const auto& [path, values] : enhanced_state) {
                                    if (conn->subscription.should_include(path)) {
                                        auto filtered_values = conn->subscription.apply_array_filters(path, values);
                                        filtered_state[path] = filtered_values;
                                    }
                                }

                                // Send filtered state
                                if (filtered_state.begin() != filtered_state.end()) {
                                    std::string filtered_state_json =
                                        monitor_state_to_osc_json(filtered_state, "filtered_state");
                                    conn->send_callback(filtered_state_json);
                                }
                            } else {
                                // No channel IDs found, use base state
                                caspar::core::monitor::state filtered_state;
                                for (const auto& [path, values] : enhanced_state) {
                                    if (conn->subscription.should_include(path)) {
                                        auto filtered_values = conn->subscription.apply_array_filters(path, values);
                                        filtered_state[path] = filtered_values;
                                    }
                                }

                                if (filtered_state.begin() != filtered_state.end()) {
                                    std::string filtered_state_json =
                                        monitor_state_to_osc_json(filtered_state, "filtered_state");
                                    conn->send_callback(filtered_state_json);
                                }
                            }
                        } else {
                            // No layer injection needed, use base state directly
                            caspar::core::monitor::state filtered_state;
                            for (const auto& [path, values] : base_state) {
                                if (conn->subscription.should_include(path)) {
                                    auto filtered_values = conn->subscription.apply_array_filters(path, values);
                                    filtered_state[path] = filtered_values;
                                }
                            }

                            if (filtered_state.begin() != filtered_state.end()) {
                                std::string filtered_state_json =
                                    monitor_state_to_osc_json(filtered_state, "filtered_state");
                                conn->send_callback(filtered_state_json);
                            }
                        }

                        conn->last_state = *state_copy; // Keep for future path-based subscriptions
                        ++it;
                    } catch (const std::exception& e) {
                        CASPAR_LOG(info) << L"WebSocket monitor: Removing failed connection " << u16(it->first) << L": "
                                         << u16(e.what());
                        it = connections_.erase(it);
                    }
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor: Error in async send: " << u16(e.what());
            }
        });
    }

    void force_disconnect_all()
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
        CASPAR_LOG(info) << L"WebSocket monitor: Force disconnected all connections";
    }

    void send_full_state_to_connection(const std::string& connection_id)
    {
        // CRITICAL: Never block any thread!
        // Post to worker pool to avoid IO context conflicts
        boost::asio::post(*worker_pool_, [this, connection_id]() {
            try {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                auto                        it = connections_.find(connection_id);
                if (it != connections_.end()) {
                    try {
                        // Send complete state with "full_state" type
                        std::string full_state_json = monitor_state_to_osc_json(current_state_, "full_state");
                        it->second->send_callback(full_state_json);
                        CASPAR_LOG(info) << L"WebSocket monitor: Sent full state to " << u16(connection_id);
                    } catch (const std::exception& e) {
                        CASPAR_LOG(error) << L"WebSocket monitor: Failed to send full state to " << u16(connection_id)
                                          << L": " << u16(e.what());
                    }
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor: Error in async full state send: " << u16(e.what());
            }
        });
    }
};

websocket_monitor_client::websocket_monitor_client(std::shared_ptr<boost::asio::io_context> context)
    : impl_(std::make_unique<impl>(std::move(context)))
{
}

websocket_monitor_client::~websocket_monitor_client() = default;

void websocket_monitor_client::add_connection(const std::string&                      connection_id,
                                              std::function<void(const std::string&)> send_callback,
                                              const subscription_config&              subscription)
{
    impl_->add_connection(connection_id, std::move(send_callback), subscription);
}

void websocket_monitor_client::remove_connection(const std::string& connection_id)
{
    impl_->remove_connection(connection_id);
}

void websocket_monitor_client::update_subscription(const std::string&         connection_id,
                                                   const subscription_config& subscription)
{
    impl_->update_subscription(connection_id, subscription);
}

void websocket_monitor_client::send(const caspar::core::monitor::state& state) { impl_->send(state); }

void websocket_monitor_client::force_disconnect_all() { impl_->force_disconnect_all(); }

void websocket_monitor_client::send_full_state_to_connection(const std::string& connection_id)
{
    impl_->send_full_state_to_connection(connection_id);
}

}}} // namespace caspar::protocol::websocket