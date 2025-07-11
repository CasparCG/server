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

#include "websocket_monitor_server.h"

#include <common/log.h>
#include <common/utf.h>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

namespace caspar { namespace protocol { namespace websocket {

// WebSocket monitor listener
class websocket_monitor_listener : public std::enable_shared_from_this<websocket_monitor_listener>
{
    net::io_context&                                               ioc_;
    tcp::acceptor                                                  acceptor_;
    std::shared_ptr<websocket_monitor_client>                      monitor_client_;
    std::atomic<bool>                                              running_{false};
    std::unordered_set<std::shared_ptr<websocket_monitor_session>> active_sessions_;
    std::mutex                                                     sessions_mutex_;

  public:
    websocket_monitor_listener(net::io_context&                          ioc,
                               tcp::endpoint                             endpoint,
                               std::shared_ptr<websocket_monitor_client> monitor_client)
        : ioc_(ioc)
        , acceptor_(ioc)
        , monitor_client_(std::move(monitor_client))
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor listener open error: " << u16(ec.message());
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor listener set_option error: " << u16(ec.message());
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor listener bind error: " << u16(ec.message());
            return;
        }

        // Start listening for connections
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor listener listen error: " << u16(ec.message());
            return;
        }

        CASPAR_LOG(info) << L"WebSocket monitor listener started on port " << endpoint.port();
    }

    void run()
    {
        running_ = true;
        do_accept();
    }

    void stop()
    {
        running_ = false;
        acceptor_.close();

        // Close all active sessions
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& session : active_sessions_) {
            if (session) {
                session->close();
            }
        }
        active_sessions_.clear();
    }

  private:
    void do_accept()
    {
        if (!running_)
            return;

        // The new connection gets its own strand
        acceptor_.async_accept(net::make_strand(ioc_),
                               beast::bind_front_handler(&websocket_monitor_listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec) {
            if (running_) {
                CASPAR_LOG(error) << L"WebSocket monitor listener accept error: " << u16(ec.message());
            }
        } else {
            // Create the session and run it
            auto session = std::make_shared<websocket_monitor_session>(
                ioc_, std::move(socket), monitor_client_, [this](std::shared_ptr<websocket_monitor_session> session) {
                    // Remove session from tracking when it's destroyed
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    active_sessions_.erase(session);
                });

            // Track the session
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                active_sessions_.insert(session);
            }

            session->run();
        }

        // Accept another connection
        do_accept();
    }
};

struct websocket_monitor_server::impl
{
    std::shared_ptr<boost::asio::io_context>    context_;
    std::shared_ptr<websocket_monitor_client>   monitor_client_;
    uint16_t                                    port_;
    std::shared_ptr<websocket_monitor_listener> listener_;
    std::atomic<bool>                           running_{false};

    impl(std::shared_ptr<boost::asio::io_context>  context,
         std::shared_ptr<websocket_monitor_client> monitor_client,
         uint16_t                                  port)
        : context_(std::move(context))
        , monitor_client_(std::move(monitor_client))
        , port_(port)
    {
    }

    void start()
    {
        if (running_)
            return;

        try {
            auto const address  = net::ip::make_address("0.0.0.0");
            auto const endpoint = tcp::endpoint{address, port_};

            // Create and launch a listening port
            listener_ = std::make_shared<websocket_monitor_listener>(*context_, endpoint, monitor_client_);
            listener_->run();

            running_ = true;
            CASPAR_LOG(info) << L"WebSocket monitor server started on port " << port_;
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"Failed to start WebSocket monitor server: " << u16(e.what());
            throw;
        }
    }

    void stop()
    {
        if (!running_)
            return;

        running_ = false;
        if (listener_) {
            listener_->stop();
        }

        // Force close all monitor connections to ensure clean shutdown
        if (monitor_client_) {
            monitor_client_->force_disconnect_all();
        }

        CASPAR_LOG(info) << L"WebSocket monitor server stopped";
    }
};

websocket_monitor_server::websocket_monitor_server(std::shared_ptr<boost::asio::io_context>  context,
                                                   std::shared_ptr<websocket_monitor_client> monitor_client,
                                                   uint16_t                                  port)
    : impl_(spl::make_shared<impl>(std::move(context), std::move(monitor_client), port))
{
}

websocket_monitor_server::~websocket_monitor_server() { impl_->stop(); }

void websocket_monitor_server::start() { impl_->start(); }

void websocket_monitor_server::stop() { impl_->stop(); }

// WebSocket monitor session method implementations
void websocket_monitor_session::handle_message(const std::string& message)
{
    try {
        // Parse JSON message
        nlohmann::json json_msg;
        try {
            json_msg = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception& e) {
            CASPAR_LOG(debug) << L"WebSocket monitor: Invalid JSON message: " << u16(e.what());
            return;
        }

        // Handle subscription command
        if (json_msg.contains("command") && json_msg["command"] == "subscribe") {
            handle_subscription_command(message);
            return;
        }

        // Log unknown messages at debug level to reduce noise
        if (!message.empty() && message != "{}" && message != "null") {
            CASPAR_LOG(debug) << L"WebSocket monitor: Received unknown message: " << u16(message);
        }
    } catch (const std::exception& e) {
        CASPAR_LOG(error) << L"WebSocket monitor: Error handling message: " << u16(e.what());
    }
}

void websocket_monitor_session::send_welcome_message()
{
    nlohmann::json welcome = {
        {"type", "welcome"},
        {"connection_id", connection_id_},
        {"message", "Welcome to CasparCG Monitor WebSocket"},
        {"instructions", "Send a subscription command to start receiving monitor data"},
        {"example",
         {{"command", "subscribe"},
          {"include", {"channel/1/*", "channel/*/mixer/volume"}},
          {"exclude", {"channel/*/mixer/audio/*"}}}},
        {"supported_patterns",
         {{"wildcards", {"* (any sequence)"}},
          {"number_ranges", {"[1-9] (single digits)", "[1-20] (numbers 1 through 20)", "[!1-5] (not 1-5)"}},
          {"alternation",
           {"[1|3|5] (specific single digits)", "[90|100|150] (specific numbers)", "[abc] (specific characters)"}},
          {"character_ranges", {"[a-z] (lowercase letters)", "[A-Z] (uppercase letters)"}},
          {"array_indexing", {"[0] (single element)", "[0:2] (range slice)", "[*] (all elements)"}},
          {"examples",
           {"* - Full state firehose (all monitor data)",
            "channel/1/* - All channel 1 data",
            "channel/*/mixer/* - All mixer data",
            "channel/[1-9]/* - Single digit channels (1-9)",
            "channel/[1-20]/* - Channels 1 through 20",
            "channel/[1|3|5]/* - Specific channels (1, 3, and 5 only)",
            "channel/*/stage/layer/[90|100|150]/* - Specific layers only",
            "channel/[!1-5]/* - All channels except 1-5",
            "channel/*/mixer/volume[0:2] - First 2 audio channels only",
            "channel/1/mixer/volume[0] - First audio channel from channel 1",
            "channel/*/stage/framerate - Framerate from all channels",
            "channel/1/mixer/volume - All volume data from channel 1"}}}},
        {"important_note", "Use '*' to subscribe to all monitor data (full firehose). Be careful with bandwidth!"},
        {"timestamp",
         std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
             .count()}};

    send(welcome.dump());
    CASPAR_LOG(info) << L"WebSocket monitor: Sent welcome message to " << u16(connection_id_);
}

void websocket_monitor_session::handle_subscription_command(const std::string& json_message)
{
    try {
        nlohmann::json      json_msg = nlohmann::json::parse(json_message);
        subscription_config subscription;

        // Parse include patterns
        if (json_msg.contains("include") && json_msg["include"].is_array()) {
            for (const auto& pattern : json_msg["include"]) {
                if (pattern.is_string()) {
                    subscription.include_patterns.emplace_back(pattern.get<std::string>());
                }
            }
        }

        // Parse exclude patterns
        if (json_msg.contains("exclude") && json_msg["exclude"].is_array()) {
            for (const auto& pattern : json_msg["exclude"]) {
                if (pattern.is_string()) {
                    subscription.exclude_patterns.emplace_back(pattern.get<std::string>());
                }
            }
        }

        // Update subscription
        if (monitor_client_) {
            monitor_client_->update_subscription(connection_id_, subscription);

            // Send confirmation
            nlohmann::json response = {{"type", "subscription_updated"},
                                       {"connection_id", connection_id_},
                                       {"include_patterns", json_msg.value("include", nlohmann::json::array())},
                                       {"exclude_patterns", json_msg.value("exclude", nlohmann::json::array())}};

            send(response.dump());

            CASPAR_LOG(info) << L"WebSocket monitor: Updated subscription for " << u16(connection_id_)
                             << L" - includes: " << json_msg.value("include", nlohmann::json::array()).size()
                             << L", excludes: " << json_msg.value("exclude", nlohmann::json::array()).size();
        }
    } catch (const std::exception& e) {
        CASPAR_LOG(error) << L"WebSocket monitor: Error handling subscription command: " << u16(e.what());

        // Send error response
        nlohmann::json error_response = {
            {"type", "error"}, {"message", "Failed to update subscription"}, {"error", e.what()}};
        send(error_response.dump());
    }
}

}}} // namespace caspar::protocol::websocket