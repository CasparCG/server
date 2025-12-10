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

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <chrono>
#include <core/monitor/monitor.h>
#include <deque>
#include <list>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tbb/concurrent_hash_map.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

namespace caspar { namespace protocol { namespace websocket {

// WebSocket monitor session implementation
class websocket_monitor_session : public std::enable_shared_from_this<websocket_monitor_session>
{
    ws::stream<beast::tcp_stream>                                   ws_;
    beast::flat_buffer                                              buffer_;
    std::shared_ptr<websocket_monitor_client>                       monitor_client_;
    std::string                                                     connection_id_;
    std::string                                                     client_address_;
    std::atomic<bool>                                               is_open_{false};
    std::unique_ptr<boost::asio::deadline_timer>                    force_close_timer_;
    boost::asio::io_context&                                        io_context_;
    std::function<void(std::shared_ptr<websocket_monitor_session>)> session_removal_callback_;
    bool                                                            write_in_flight_{false};
    std::atomic<bool>                                               removal_callback_called_{false};

    // Circuit breaker configuration
    static constexpr int SUSTAINED_FAILURE_SECONDS = 20; // Close connection after 20 seconds of failures

    // Statistics tracking
    std::atomic<int>                      total_messages_sent_{0};
    std::atomic<int>                      total_messages_dropped_{0};
    std::chrono::steady_clock::time_point first_failure_time_;
    std::chrono::steady_clock::time_point last_drop_time_;
    bool                                  has_failures_{false};
    bool                                  has_drops_{false};

  public:
    explicit websocket_monitor_session(
        boost::asio::io_context&                                        io_context,
        tcp::socket&&                                                   socket,
        std::shared_ptr<websocket_monitor_client>                       monitor_client,
        std::function<void(std::shared_ptr<websocket_monitor_session>)> session_removal_callback = nullptr)
        : ws_(std::move(socket))
        , monitor_client_(std::move(monitor_client))
        , connection_id_(generate_connection_id())
        , io_context_(io_context)
        , session_removal_callback_(std::move(session_removal_callback))
    {
        // Get client address from socket
        try {
            auto endpoint   = ws_.next_layer().socket().remote_endpoint();
            client_address_ = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session: Failed to get client address: " << u16(e.what());
            client_address_ = "unknown";
        }
    }

    ~websocket_monitor_session()
    {
        // Ensure we're marked as closed
        is_open_ = false;

        // Cancel any pending timers
        if (force_close_timer_) {
            try {
                force_close_timer_->cancel();
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor session timer cancel error: " << u16(e.what());
            }
        }

        // Remove from monitor client if still connected
        if (monitor_client_) {
            try {
                monitor_client_->remove_connection(connection_id_);
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor session destructor cleanup error: " << u16(e.what());
            }
        }

        // Notify listener that this session is being destroyed (only once)
        if (session_removal_callback_ && !removal_callback_called_.exchange(true)) {
            try {
                // Post to IO context to avoid calling callback during destruction
                boost::asio::post(io_context_, [this]() {
                    try {
                        if (session_removal_callback_) {
                            session_removal_callback_(shared_from_this());
                        }
                    } catch (const std::exception& e) {
                        CASPAR_LOG(error) << L"WebSocket monitor session removal callback error: " << u16(e.what());
                    }
                });
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor session removal callback error: " << u16(e.what());
            }
        }
    }

    void run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));

        // Enable permessage-deflate compression (RFC 7692) for better performance
        ws::permessage_deflate pmd;
        pmd.server_enable = true;
        pmd.client_enable = true;
        ws_.set_option(pmd);

        // Set a decorator to change the Server of the handshake
        ws_.set_option(ws::stream_base::decorator([](ws::response_type& res) {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " CasparCG-Monitor");
        }));

        // Accept the websocket handshake
        auto weak_self = std::weak_ptr<websocket_monitor_session>(this->shared_from_this());
        ws_.async_accept([weak_self](beast::error_code ec) {
            auto self = weak_self.lock();
            if (self) {
                self->on_accept(ec);
            }
        });
    }

    void send(const std::string& message)
    {
        if (message.empty()) {
            return; // Don't send empty messages
        }

        // Post the work to the correct executor to preserve ordering
        auto weak_self = std::weak_ptr<websocket_monitor_session>(this->shared_from_this());
        net::post(ws_.get_executor(), [weak_self, msg = std::string(message)]() mutable {
            auto self = weak_self.lock();
            if (!self)
                return;

            try {
                if (!self->is_open_)
                    return;

                // If a write is currently in flight, drop the message
                if (self->write_in_flight_) {
                    auto dropped_count = self->total_messages_dropped_.fetch_add(1) + 1;

                    // Log first drop
                    if (!self->has_drops_) {
                        self->has_drops_      = true;
                        self->last_drop_time_ = std::chrono::steady_clock::now();
                        CASPAR_LOG(warning)
                            << L"WebSocket monitor: client " << u16(self->connection_id_) << L" ("
                            << u16(self->client_address_) << L") dropped first message - client may be slow";
                    } else {
                        // Update last drop time on subsequent drops
                        self->last_drop_time_ = std::chrono::steady_clock::now();
                    }

                    // Log periodically (every 100 drops)
                    if (dropped_count % 100 == 0) {
                        CASPAR_LOG(warning)
                            << L"WebSocket monitor: client " << u16(self->connection_id_) << L" ("
                            << u16(self->client_address_) << L") has dropped " << dropped_count << L" messages";
                    }

                    return;
                }

                // No write in flight – send immediately
                self->write_in_flight_ = true;
                auto out               = std::make_shared<std::string>(std::move(msg));
                self->ws_.text(true);
                self->ws_.async_write(net::buffer(*out), [weak_self, out](beast::error_code ec, std::size_t bytes) {
                    auto self = weak_self.lock();
                    if (self) {
                        self->on_write(ec, bytes);
                    }
                });
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor session send error: " << u16(e.what());
                self->is_open_ = false;
                if (self->monitor_client_)
                    self->monitor_client_->remove_connection(self->connection_id_);
            }
        });
    }

    bool is_connection_open() const { return is_open_; }

    void close()
    {
        if (is_open_) {
            is_open_ = false;

            // Cancel force close timer if it exists
            if (force_close_timer_) {
                try {
                    force_close_timer_->cancel();
                } catch (const std::exception& e) {
                    CASPAR_LOG(error) << L"WebSocket monitor session timer cancel error: " << u16(e.what());
                }
            }

            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e) {
                    CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e.what());
                }
            }

            try {
                // Graceful close
                auto weak_self = std::weak_ptr<websocket_monitor_session>(this->shared_from_this());
                ws_.async_close(ws::close_code::normal, [weak_self](beast::error_code ec) {
                    auto self = weak_self.lock();
                    if (self) {
                        if (ec) {
                            CASPAR_LOG(error) << L"WebSocket monitor session close error: " << u16(ec.message());
                        } else {
                            CASPAR_LOG(info)
                                << L"WebSocket monitor session closed gracefully: " << u16(self->connection_id_)
                                << L" (" << u16(self->client_address_) << L")";
                        }
                    }
                });

                // Start force close timer (1 second)
                if (!force_close_timer_) {
                    force_close_timer_ = std::make_unique<boost::asio::deadline_timer>(io_context_);
                }
                force_close_timer_->expires_from_now(boost::posix_time::seconds(1));
                force_close_timer_->async_wait([weak_self](const boost::system::error_code& ec) {
                    auto self = weak_self.lock();
                    if (self && !ec && self->ws_.is_open()) {
                        CASPAR_LOG(warning) << L"WebSocket monitor session forcefully closed: "
                                            << u16(self->connection_id_) << L" (" << u16(self->client_address_) << L")";
                        self->ws_.next_layer().close();
                    }
                });
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"WebSocket monitor session close error: " << u16(e.what());
                // Force close the underlying socket
                try {
                    ws_.next_layer().close();
                } catch (const std::exception& e2) {
                    CASPAR_LOG(error) << L"WebSocket monitor session force close error: " << u16(e2.what());
                }
            }
        }
    }

  private:
    std::string generate_connection_id()
    {
        try {
            static std::atomic<uint64_t> counter{0};
            return "monitor_" + std::to_string(counter++);
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session connection ID generation error: " << u16(e.what());
            // Fallback to a simple ID
            return "monitor_fallback";
        }
    }

    void on_accept(beast::error_code ec)
    {
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session accept error: " << u16(ec.message());
            return;
        }

        try {
            is_open_ = true;

            // Register this connection with the monitor client
            auto weak_self = std::weak_ptr<websocket_monitor_session>(this->shared_from_this());
            monitor_client_->add_connection(connection_id_, [weak_self](const std::string& message) {
                auto self = weak_self.lock();
                if (self) {
                    self->send(message);
                }
            });

            CASPAR_LOG(info) << L"WebSocket monitor session connected: " << u16(connection_id_) << L" ("
                             << u16(client_address_) << L")";

            // Send welcome message
            send_welcome_message();

            // Start reading for potential close messages
            do_read();
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session on_accept error: " << u16(e.what());
            is_open_ = false;
            // Try to clean up if possible
            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e2) {
                    CASPAR_LOG(error) << L"WebSocket monitor session cleanup error: " << u16(e2.what());
                }
            }
        }
    }

    void do_read()
    {
        try {
            // Read a message into our buffer
            auto weak_self = std::weak_ptr<websocket_monitor_session>(this->shared_from_this());
            ws_.async_read(buffer_, [weak_self](beast::error_code ec, std::size_t bytes_transferred) {
                auto self = weak_self.lock();
                if (self) {
                    self->on_read(ec, bytes_transferred);
                }
            });
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session do_read error: " << u16(e.what());
            is_open_ = false;
            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e2) {
                    CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e2.what());
                }
            }
        }
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == ws::error::closed) {
            // Connection closed normally
            CASPAR_LOG(info) << L"WebSocket monitor session closed: " << u16(connection_id_) << L" ("
                             << u16(client_address_) << L")";
            is_open_ = false;
            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e) {
                    CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e.what());
                }
            }
            return;
        }

        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session read error: " << u16(ec.message());
            is_open_ = false;
            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e) {
                    CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e.what());
                }
            }
            return;
        }

        // Handle incoming messages
        try {
            std::string message = beast::buffers_to_string(buffer_.data());
            if (!message.empty()) {
                CASPAR_LOG(debug) << L"WebSocket monitor: Received message: " << u16(message);
                handle_message(message);
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session message handling error: " << u16(e.what());
            // Don't disconnect on message handling errors, just log and continue
        }

        // Clear buffer and continue reading
        try {
            buffer_.consume(buffer_.size());
            do_read();
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session buffer handling error: " << u16(e.what());
            is_open_ = false;
            if (monitor_client_) {
                try {
                    monitor_client_->remove_connection(connection_id_);
                } catch (const std::exception& e2) {
                    CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e2.what());
                }
            }
        }
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session write error: " << u16(ec.message());

            // Track failure timing
            auto now = std::chrono::steady_clock::now();
            if (!has_failures_) {
                first_failure_time_ = now;
                has_failures_       = true;
                CASPAR_LOG(warning) << L"WebSocket monitor: client " << u16(connection_id_) << L" ("
                                    << u16(client_address_)
                                    << L") experiencing write failures - starting failure timer";
            }

            // Check if we've had sustained failures for too long
            auto failure_duration = std::chrono::duration_cast<std::chrono::seconds>(now - first_failure_time_).count();

            if (failure_duration >= SUSTAINED_FAILURE_SECONDS) {
                CASPAR_LOG(warning) << L"WebSocket monitor: client " << u16(connection_id_) << L" ("
                                    << u16(client_address_) << L") had sustained failures for " << failure_duration
                                    << L" seconds. Closing connection.";
                is_open_ = false;
                if (monitor_client_) {
                    try {
                        monitor_client_->remove_connection(connection_id_);
                    } catch (const std::exception& e) {
                        CASPAR_LOG(error) << L"WebSocket monitor session remove connection error: " << u16(e.what());
                    }
                }
                return;
            }

            // Move on - don't retry failed message
            write_in_flight_ = false;
            return;
        }

        // Write completed successfully
        total_messages_sent_.fetch_add(1);

        // Reset failure tracking on success
        if (has_failures_) {
            CASPAR_LOG(info) << L"WebSocket monitor: client " << u16(connection_id_) << L" (" << u16(client_address_)
                             << L") recovered from write failures";
            has_failures_ = false;
        }

        // Reset drop tracking on successful write (only if we haven't dropped recently)
        if (has_drops_) {
            auto total_dropped        = total_messages_dropped_.load();
            auto now                  = std::chrono::steady_clock::now();
            auto time_since_last_drop = std::chrono::duration_cast<std::chrono::seconds>(now - last_drop_time_).count();

            // Only mark as recovered if we haven't dropped any messages in the last 5 seconds
            if (time_since_last_drop >= 5) {
                CASPAR_LOG(info) << L"WebSocket monitor: client " << u16(connection_id_) << L" ("
                                 << u16(client_address_) << L") recovered - stopped dropping messages "
                                 << L"(total dropped: " << total_dropped << L")";
                has_drops_ = false;
                total_messages_dropped_.store(0); // Reset the counter
            }
        }

        write_in_flight_ = false;
    }

    // Handle incoming JSON messages from the client
    void handle_message(const std::string& message);

    // Send welcome message to new client
    void send_welcome_message();

    // Handle subscription command from client
    void handle_subscription_command(const std::string& json_message);

    // Handle request_full_state command from client
    void handle_request_full_state_command();
};

// WebSocket monitor listener
class websocket_monitor_listener : public std::enable_shared_from_this<websocket_monitor_listener>
{
    net::io_context&                          ioc_;
    tcp::acceptor                             acceptor_;
    std::shared_ptr<websocket_monitor_client> monitor_client_;
    std::atomic<bool>                         running_{false};
    // Lock-free session tracking (no mutex needed)
    tbb::concurrent_hash_map<std::shared_ptr<websocket_monitor_session>, bool> active_sessions_;

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

        // Close all active sessions (lock-free)
        for (tbb::concurrent_hash_map<std::shared_ptr<websocket_monitor_session>, bool>::iterator it =
                 active_sessions_.begin();
             it != active_sessions_.end();
             ++it) {
            if (it->first) {
                it->first->close();
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
        auto weak_self = std::weak_ptr<websocket_monitor_listener>(this->shared_from_this());
        acceptor_.async_accept(net::make_strand(ioc_), [weak_self](beast::error_code ec, tcp::socket socket) {
            auto self = weak_self.lock();
            if (self) {
                self->on_accept(ec, std::move(socket));
            }
        });
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
                ioc_,
                std::move(socket),
                monitor_client_,
                [this, weak_self = std::weak_ptr<websocket_monitor_listener>(this->shared_from_this())](
                    std::shared_ptr<websocket_monitor_session> session) {
                    // Remove session from tracking when it's destroyed (lock-free)
                    auto self = weak_self.lock();
                    if (self) {
                        tbb::concurrent_hash_map<std::shared_ptr<websocket_monitor_session>, bool>::accessor acc;
                        if (self->active_sessions_.find(acc, session)) {
                            self->active_sessions_.erase(acc);
                        }
                    }
                });

            // Track the session (lock-free)
            tbb::concurrent_hash_map<std::shared_ptr<websocket_monitor_session>, bool>::accessor acc;
            active_sessions_.insert(acc, session);
            acc->second = true;

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

        // Handle request_full_state command
        if (json_msg.contains("command") && json_msg["command"] == "request_full_state") {
            handle_request_full_state_command();
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
        {"instructions",
         "Send a subscription command to start receiving monitor data, or request_full_state for a one-time snapshot"},
        {"commands",
         {{"subscribe",
           {{"description", "Set up ongoing filtered monitor data subscription"},
            {"example",
             {{"command", "subscribe"},
              {"include", {"channel/1/*", "channel/*/mixer/audio/volume"}},
              {"exclude", {"channel/*/mixer/audio/*"}}}}}},
          {"request_full_state",
           {{"description", "Get a one-time snapshot of all current monitor data"},
            {"example", {{"command", "request_full_state"}}}}}}},
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

void websocket_monitor_session::handle_request_full_state_command()
{
    try {
        if (monitor_client_) {
            // Send full state immediately without changing subscription
            monitor_client_->send_full_state_to_connection(connection_id_);
            CASPAR_LOG(info) << L"WebSocket monitor: Sent full state to " << u16(connection_id_);
        }
    } catch (const std::exception& e) {
        CASPAR_LOG(error) << L"WebSocket monitor: Error handling request_full_state command: " << u16(e.what());

        // Send error response
        nlohmann::json error_response = {
            {"type", "error"}, {"message", "Failed to send full state"}, {"error", e.what()}};
        send(error_response.dump());
    }
}

}}} // namespace caspar::protocol::websocket