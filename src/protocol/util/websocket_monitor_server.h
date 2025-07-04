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

#pragma once

#include "websocket_monitor_client.h"

#include <atomic>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <common/memory.h>
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

class websocket_monitor_session : public std::enable_shared_from_this<websocket_monitor_session>
{
    ws::stream<beast::tcp_stream>                                   ws_;
    beast::flat_buffer                                              buffer_;
    std::shared_ptr<websocket_monitor_client>                       monitor_client_;
    std::string                                                     connection_id_;
    std::shared_ptr<void>                                           subscription_token_;
    std::atomic<bool>                                               is_open_{false};
    std::unique_ptr<boost::asio::deadline_timer>                    force_close_timer_;
    boost::asio::io_context&                                        io_context_;
    std::function<void(std::shared_ptr<websocket_monitor_session>)> session_removal_callback_;

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
    }

    ~websocket_monitor_session()
    {
        // Ensure we're marked as closed
        is_open_ = false;

        // Cancel any pending timers
        if (force_close_timer_) {
            force_close_timer_->cancel();
        }

        // Remove from monitor client if still connected
        if (monitor_client_) {
            monitor_client_->remove_connection(connection_id_);
        }

        // Notify listener that this session is being destroyed
        if (session_removal_callback_) {
            session_removal_callback_(shared_from_this());
        }
    }

    void run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(ws::stream_base::decorator([](ws::response_type& res) {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " CasparCG-Monitor");
        }));

        // Accept the websocket handshake
        ws_.async_accept(beast::bind_front_handler(&websocket_monitor_session::on_accept, shared_from_this()));
    }

    void send(const std::string& message)
    {
        if (message.empty()) {
            return; // Don't send empty messages
        }

        // Post the work to the correct executor
        net::post(ws_.get_executor(), [self = shared_from_this(), message]() {
            if (self->is_open_) {
                self->ws_.async_write(net::buffer(message),
                                      beast::bind_front_handler(&websocket_monitor_session::on_write, self));
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
                force_close_timer_->cancel();
            }

            if (monitor_client_) {
                monitor_client_->remove_connection(connection_id_);
            }

            // Graceful close
            ws_.async_close(ws::close_code::normal, [self = shared_from_this()](beast::error_code ec) {
                if (ec) {
                    CASPAR_LOG(error) << L"WebSocket monitor session close error: " << u16(ec.message());
                } else {
                    CASPAR_LOG(info) << L"WebSocket monitor session closed gracefully: " << u16(self->connection_id_);
                }
            });

            // Start force close timer (1 second)
            if (!force_close_timer_) {
                force_close_timer_ = std::make_unique<boost::asio::deadline_timer>(io_context_);
            }
            force_close_timer_->expires_from_now(boost::posix_time::seconds(1));
            force_close_timer_->async_wait([self = shared_from_this()](const boost::system::error_code& ec) {
                if (!ec && self->ws_.is_open()) {
                    CASPAR_LOG(warning) << L"WebSocket monitor session forcefully closed: "
                                        << u16(self->connection_id_);
                    self->ws_.next_layer().close();
                }
            });
        }
    }

  private:
    std::string generate_connection_id()
    {
        static std::atomic<uint64_t> counter{0};
        return "monitor_" + std::to_string(counter++);
    }

    void on_accept(beast::error_code ec)
    {
        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session accept error: " << u16(ec.message());
            return;
        }

        is_open_ = true;

        // Register this connection with the monitor client
        auto weak_self = std::weak_ptr<websocket_monitor_session>(shared_from_this());
        monitor_client_->add_connection(connection_id_, [weak_self](const std::string& message) {
            auto self = weak_self.lock();
            if (self) {
                self->send(message);
            }
        });

        // Get subscription token
        subscription_token_ = monitor_client_->get_subscription_token(connection_id_);

        CASPAR_LOG(info) << L"WebSocket monitor session connected: " << u16(connection_id_);

        // Start reading for potential close messages
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(buffer_, beast::bind_front_handler(&websocket_monitor_session::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == ws::error::closed) {
            // Connection closed normally
            CASPAR_LOG(info) << L"WebSocket monitor session closed: " << u16(connection_id_);
            is_open_ = false;
            if (monitor_client_) {
                monitor_client_->remove_connection(connection_id_);
            }
            return;
        }

        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session read error: " << u16(ec.message());
            is_open_ = false;
            if (monitor_client_) {
                monitor_client_->remove_connection(connection_id_);
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
        }

        // Clear buffer and continue reading
        buffer_.consume(buffer_.size());
        do_read();
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            CASPAR_LOG(error) << L"WebSocket monitor session write error: " << u16(ec.message());
            is_open_ = false;
            if (monitor_client_) {
                monitor_client_->remove_connection(connection_id_);
            }
            return;
        }

        // Continue processing (write completed successfully)
    }

    // Handle incoming JSON messages from the client
    void handle_message(const std::string& message)
    {
        try {
            // Simple but robust JSON parsing - look for command field
            size_t command_pos = message.find("\"command\"");
            if (command_pos != std::string::npos) {
                // Find the value after "command":
                size_t colon_pos = message.find(':', command_pos);
                if (colon_pos != std::string::npos) {
                    // Skip whitespace and quotes
                    size_t value_start = colon_pos + 1;
                    while (
                        value_start < message.length() &&
                        (message[value_start] == ' ' || message[value_start] == '\t' || message[value_start] == '\"')) {
                        value_start++;
                    }

                    if (value_start < message.length()) {
                        // Check if the value starts with "request_full_state" (case insensitive)
                        std::string value = message.substr(value_start, 19);
                        if (value.length() == 19) {
                            // Convert to lowercase for case-insensitive comparison
                            std::string lower_value = value;
                            std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
                            if (lower_value == "request_full_state") {
                                // Request full state for this connection
                                if (monitor_client_) {
                                    monitor_client_->request_full_state(connection_id_);
                                    CASPAR_LOG(info) << L"WebSocket monitor: Full state requested by connection "
                                                     << u16(connection_id_);
                                }
                                return;
                            }
                        }
                    }
                }
            }

            // Log unknown messages at debug level to reduce noise
            if (!message.empty() && message != "{}" && message != "null") {
                CASPAR_LOG(debug) << L"WebSocket monitor: Received unknown message: " << u16(message);
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor: Error handling message: " << u16(e.what());
        }
    }
};

class websocket_monitor_server
{
    websocket_monitor_server(const websocket_monitor_server&)            = delete;
    websocket_monitor_server& operator=(const websocket_monitor_server&) = delete;

  public:
    websocket_monitor_server(std::shared_ptr<boost::asio::io_service>  service,
                             std::shared_ptr<websocket_monitor_client> monitor_client,
                             uint16_t                                  port);

    ~websocket_monitor_server();

    void start();
    void stop();

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}}} // namespace caspar::protocol::websocket