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
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <common/memory.h>
#include <deque>
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
    std::atomic<bool>                                               is_open_{false};
    std::unique_ptr<boost::asio::deadline_timer>                    force_close_timer_;
    boost::asio::io_context&                                        io_context_;
    std::function<void(std::shared_ptr<websocket_monitor_session>)> session_removal_callback_;
    std::deque<std::string>                                         pending_msgs_;
    bool                                                            write_in_flight_{false};

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

        // Notify listener that this session is being destroyed
        if (session_removal_callback_) {
            try {
                session_removal_callback_(shared_from_this());
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
        ws_.async_accept(beast::bind_front_handler(&websocket_monitor_session::on_accept, shared_from_this()));
    }

    void send(const std::string& message)
    {
        if (message.empty()) {
            return; // Don't send empty messages
        }

        // Post the work to the correct executor to preserve ordering
        net::post(ws_.get_executor(), [self = shared_from_this(), msg = std::string(message)]() mutable {
            try {
                if (!self->is_open_)
                    return;

                // If a write is currently in flight, queue the message
                if (self->write_in_flight_) {
                    if (self->pending_msgs_.size() >= 300) {
                        CASPAR_LOG(warning) << L"WebSocket monitor: client " << u16(self->connection_id_)
                                            << L" too slow (queue overflow). Closing.";
                        self->close();
                        return;
                    }

                    // Temporary debug logging of queueing behaviour
                    if (self->pending_msgs_.empty()) {
                        CASPAR_LOG(debug) << L"WebSocket monitor: starting to queue messages for client "
                                          << u16(self->connection_id_);
                    }

                    // Simply queue the message; if the client stays slow the hard limit will close it
                    self->pending_msgs_.push_back(std::move(msg));

                    if (self->pending_msgs_.size() % 50 == 0) {
                        CASPAR_LOG(debug) << L"WebSocket monitor: queue size now " << self->pending_msgs_.size()
                                          << L" for client " << u16(self->connection_id_);
                    }
                    return;
                }

                // No write in flight – send immediately
                self->write_in_flight_ = true;
                auto out               = std::make_shared<std::string>(std::move(msg));
                self->ws_.text(true);
                self->ws_.async_write(net::buffer(*out), [self, out](beast::error_code ec, std::size_t bytes) {
                    self->on_write(ec, bytes);
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
                ws_.async_close(ws::close_code::normal, [self = shared_from_this()](beast::error_code ec) {
                    if (ec) {
                        CASPAR_LOG(error) << L"WebSocket monitor session close error: " << u16(ec.message());
                    } else {
                        CASPAR_LOG(info) << L"WebSocket monitor session closed gracefully: "
                                         << u16(self->connection_id_);
                    }
                });

                // Start force close timer (1 second)
                if (!force_close_timer_) {
                    force_close_timer_ = std::make_unique<boost::asio::deadline_timer>(io_context_);
                }
                force_close_timer_->expires_from_now(boost::posix_time::seconds(1));
                force_close_timer_->async_wait([self = shared_from_this()](const boost::system::error_code& ec) {
                    if (!ec && self->ws_.is_open()) {
                        CASPAR_LOG(warning)
                            << L"WebSocket monitor session forcefully closed: " << u16(self->connection_id_);
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
            auto weak_self = std::weak_ptr<websocket_monitor_session>(shared_from_this());
            monitor_client_->add_connection(connection_id_, [weak_self](const std::string& message) {
                auto self = weak_self.lock();
                if (self) {
                    self->send(message);
                }
            });

            CASPAR_LOG(info) << L"WebSocket monitor session connected: " << u16(connection_id_);

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
            ws_.async_read(buffer_, beast::bind_front_handler(&websocket_monitor_session::on_read, shared_from_this()));
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
            CASPAR_LOG(info) << L"WebSocket monitor session closed: " << u16(connection_id_);
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

        // Write completed successfully – send next queued message if any
        try {
            if (!pending_msgs_.empty()) {
                std::string next = std::move(pending_msgs_.front());
                pending_msgs_.pop_front();
                auto out = std::make_shared<std::string>(std::move(next));
                ws_.text(true);
                ws_.async_write(net::buffer(*out),
                                [self = shared_from_this(), out](beast::error_code ec, std::size_t bytes) {
                                    self->on_write(ec, bytes);
                                });
            } else {
                write_in_flight_ = false;
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"WebSocket monitor session queue send error: " << u16(e.what());
            close();
        }
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
                        // Extract the command value up to the next double quote or whitespace
                        size_t value_end = value_start;
                        while (value_end < message.length() && message[value_end] != '"' && message[value_end] != ' ' &&
                               message[value_end] != '\t' && message[value_end] != '\n' && message[value_end] != '\r') {
                            ++value_end;
                        }

                        std::string value = message.substr(value_start, value_end - value_start);

                        // Convert to lowercase for case-insensitive comparison
                        std::string lower_value = value;
                        std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

                        if (lower_value == "request_full_state") {
                            // Request full state for this connection
                            if (monitor_client_) {
                                monitor_client_->force_full_state();
                                CASPAR_LOG(info)
                                    << L"WebSocket monitor: Full state requested by connection " << u16(connection_id_);
                            }
                            return;
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
    websocket_monitor_server(std::shared_ptr<boost::asio::io_context>  context,
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