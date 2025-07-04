/*
 * Copyright (c) 2024 CasparCG (www.casparcg.com).
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
 */

#include "../StdAfx.h"

#include "websocket_server.h"

#include <common/log.h>
#include <common/utf.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/option.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp           = net::ip::tcp;

namespace caspar { namespace IO {

// Convert monitor state to JSON string
std::string monitor_state_to_json(const core::monitor::state& state)
{
    boost::property_tree::wptree json_tree;

    // Add timestamp
    json_tree.put(
        L"timestamp",
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    // Convert monitor state to JSON structure
    // The monitor state contains various performance and status information
    boost::property_tree::wptree channels_tree;

    for (const auto& channel_pair : state) {
        const auto& path   = channel_pair.first;
        const auto& values = channel_pair.second;

        boost::property_tree::wptree channel_tree;
        for (size_t i = 0; i < values.size(); ++i) {
            const auto& value = values[i];

            // Convert boost::variant to appropriate JSON value
            if (auto int_val = boost::get<int>(&value)) {
                channel_tree.put(u16(path + "/" + std::to_string(i)), *int_val);
            } else if (auto double_val = boost::get<double>(&value)) {
                channel_tree.put(u16(path + "/" + std::to_string(i)), *double_val);
            } else if (auto string_val = boost::get<std::string>(&value)) {
                channel_tree.put(u16(path + "/" + std::to_string(i)), u16(*string_val));
            } else if (auto wstring_val = boost::get<std::wstring>(&value)) {
                channel_tree.put(u16(path + "/" + std::to_string(i)), *wstring_val);
            } else if (auto bool_val = boost::get<bool>(&value)) {
                channel_tree.put(u16(path + "/" + std::to_string(i)), *bool_val);
            }
        }

        if (!channel_tree.empty()) {
            channels_tree.add_child(u16(path), channel_tree);
        }
    }

    json_tree.add_child(L"data", channels_tree);

    // Convert to JSON string
    std::wstringstream json_stream;
    boost::property_tree::write_json(json_stream, json_tree, false);
    return u8(json_stream.str());
}

// WebSocket session for AMCP connections
class websocket_amcp_session : public spl::enable_shared_from_this<websocket_amcp_session>
{
    websocket::stream<tcp::socket>                ws_;
    std::string                                   address_;
    std::map<std::wstring, std::shared_ptr<void>> lifecycle_objects_;
    std::mutex                                    mutex_;
    beast::flat_buffer                            buffer_;
    std::shared_ptr<protocol_strategy<wchar_t>>   strategy_;

    // Client connection holder for protocol strategy
    class connection_holder : public client_connection<wchar_t>
    {
        std::weak_ptr<websocket_amcp_session> session_;

      public:
        explicit connection_holder(std::weak_ptr<websocket_amcp_session> session)
            : session_(std::move(session))
        {
        }

        void send(std::basic_string<wchar_t>&& data, bool skip_log) override
        {
            auto session = session_.lock();
            if (session) {
                session->send(std::move(data), skip_log);
            }
        }

        void disconnect() override
        {
            auto session = session_.lock();
            if (session) {
                session->disconnect();
            }
        }

        std::wstring address() const override
        {
            auto session = session_.lock();
            if (session) {
                return session->address();
            }
            return L"[destroyed-session]";
        }

        void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) override
        {
            auto session = session_.lock();
            if (session) {
                session->add_lifecycle_bound_object(key, lifecycle_bound);
            }
        }

        std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) override
        {
            auto session = session_.lock();
            if (session) {
                return session->remove_lifecycle_bound_object(key);
            }
            return std::shared_ptr<void>();
        }
    };

  public:
    websocket_amcp_session(tcp::socket socket)
        : ws_(std::move(socket))
    {
        auto endpoint = ws_.next_layer().remote_endpoint();
        address_      = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void start(const protocol_strategy_factory<wchar_t>::ptr& strategy_factory)
    {
        // Create protocol strategy using the factory
        auto client_connection = spl::make_shared<connection_holder>(shared_from_this());
        strategy_              = strategy_factory->create(client_connection);

        // Enable permessage-deflate compression (RFC 7692) so that
        // WebSocket clients can negotiate compressed messages during the
        // opening handshake. This must be set before calling async_accept().
        websocket::permessage_deflate pmd;
        pmd.server_enable = true; // Allow the server to send compressed messages
        pmd.client_enable = true; // Allow the server to accept compressed messages
        ws_.set_option(pmd);

        // Accept the websocket handshake
        ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                self->do_read();
            } else {
                CASPAR_LOG(error) << L"WebSocket AMCP accept failed: " << u16(ec.message());
            }
        });
    }

    void send(std::wstring&& data, bool skip_log)
    {
        auto self = shared_from_this();
        net::post(ws_.get_executor(), [self, data = std::move(data), skip_log]() mutable {
            try {
                if (self->ws_.is_open()) {
                    std::string utf8_data = u8(data);
                    self->ws_.write(net::buffer(utf8_data));

                    if (!skip_log) {
                        if (data.length() < 512) {
                            boost::replace_all(data, L"\n", L"\\n");
                            boost::replace_all(data, L"\r", L"\\r");
                            CASPAR_LOG(info) << L"Sent WebSocket message to " << u16(self->address_) << L": " << data;
                        } else {
                            CASPAR_LOG(info) << L"Sent WebSocket message (>512 bytes) to " << u16(self->address_);
                        }
                    }
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"Failed to send WebSocket message: " << u16(e.what());
            }
        });
    }

    void disconnect()
    {
        auto self = shared_from_this();
        net::post(ws_.get_executor(), [self]() {
            try {
                if (self->ws_.is_open()) {
                    self->ws_.close(websocket::close_code::normal);
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"Failed to close WebSocket connection: " << u16(e.what());
            }
        });
    }

    std::wstring address() const { return u16(address_); }

    void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lifecycle_objects_[key] = lifecycle_bound;
    }

    std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto                        it = lifecycle_objects_.find(key);
        if (it != lifecycle_objects_.end()) {
            auto result = it->second;
            lifecycle_objects_.erase(it);
            return result;
        }
        return std::shared_ptr<void>();
    }

  private:
    void do_read()
    {
        auto self = shared_from_this();
        ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                try {
                    std::string message = beast::buffers_to_string(self->buffer_.data());
                    self->buffer_.consume(bytes_transferred);

                    // Convert UTF-8 message to wide string for AMCP protocol
                    std::wstring wide_msg = u16(message);

                    // Debug logging
                    CASPAR_LOG(info) << L"WebSocket received message: [" << wide_msg << L"] (length: "
                                     << wide_msg.length() << L")";

                    if (self->strategy_.get()) {
                        // Ensure the message ends with AMCP delimiter
                        if (!wide_msg.empty() && !boost::algorithm::ends_with(wide_msg, L"\r\n")) {
                            wide_msg += L"\r\n";
                        }
                        self->strategy_->parse(wide_msg);
                    }

                    // Continue reading
                    self->do_read();
                } catch (const std::exception& e) {
                    CASPAR_LOG(error) << L"Error processing AMCP WebSocket message: " << u16(e.what());
                }
            } else if (ec != websocket::error::closed) {
                CASPAR_LOG(error) << L"WebSocket AMCP read error: " << u16(ec.message());
            }
        });
    }
};

// WebSocket session for monitor connections
class websocket_monitor_session : public spl::enable_shared_from_this<websocket_monitor_session>
{
    websocket::stream<tcp::socket> ws_;
    std::string                    address_;

  public:
    websocket_monitor_session(tcp::socket socket)
        : ws_(std::move(socket))
    {
        auto endpoint = ws_.next_layer().remote_endpoint();
        address_      = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void start()
    {
        // Enable permessage-deflate compression for monitor sessions as well
        websocket::permessage_deflate pmd;
        pmd.server_enable = true;
        pmd.client_enable = true;
        ws_.set_option(pmd);

        // Accept the websocket handshake
        ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                CASPAR_LOG(info) << L"WebSocket Monitor client connected: " << u16(self->address_);
                self->do_read();
            } else {
                CASPAR_LOG(error) << L"WebSocket Monitor accept failed: " << u16(ec.message());
            }
        });
    }

    void send_monitor_data(const std::string& json_data)
    {
        auto self = shared_from_this();
        net::post(ws_.get_executor(), [self, json_data]() {
            try {
                if (self->ws_.is_open()) {
                    self->ws_.write(net::buffer(json_data));
                }
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << L"Failed to send monitor data to WebSocket client: " << u16(e.what());
            }
        });
    }

    bool is_open() const { return ws_.is_open(); }

    std::string address() const { return address_; }

  private:
    void do_read()
    {
        auto self = shared_from_this();
        // Monitor connections are read-only, but we still need to handle close events
        ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t) {
            if (ec == websocket::error::closed) {
                CASPAR_LOG(info) << L"WebSocket Monitor client disconnected: " << u16(self->address_);
            } else if (ec) {
                CASPAR_LOG(error) << L"WebSocket Monitor read error: " << u16(ec.message());
            } else {
                // Ignore incoming messages from monitor clients
                self->buffer_.clear();
                self->do_read();
            }
        });
    }

    beast::flat_buffer buffer_;
};

struct websocket_server::impl : public spl::enable_shared_from_this<websocket_server::impl>
{
    std::shared_ptr<boost::asio::io_context> context_;
    protocol_strategy_factory<wchar_t>::ptr  amcp_protocol_factory_;
    uint16_t                                 amcp_port_;
    uint16_t                                 monitor_port_;

    tcp::acceptor amcp_acceptor_;
    tcp::acceptor monitor_acceptor_;

    std::set<spl::shared_ptr<websocket_monitor_session>>                                           monitor_sessions_;
    std::vector<std::function<std::pair<std::wstring, std::shared_ptr<void>>(const std::string&)>> lifecycle_factories_;

    std::mutex sessions_mutex_;

    impl(std::shared_ptr<boost::asio::io_context>       context,
         const protocol_strategy_factory<wchar_t>::ptr& amcp_protocol_factory,
         uint16_t                                       amcp_port,
         uint16_t                                       monitor_port)
        : context_(context)
        , amcp_protocol_factory_(amcp_protocol_factory)
        , amcp_port_(amcp_port)
        , monitor_port_(monitor_port)
        , amcp_acceptor_(*context)
        , monitor_acceptor_(*context)
    {
    }

    void start()
    {
        try {
            // Start AMCP server
            tcp::endpoint amcp_endpoint{tcp::v4(), amcp_port_};
            amcp_acceptor_.open(amcp_endpoint.protocol());
            amcp_acceptor_.set_option(net::socket_base::reuse_address(true));
            amcp_acceptor_.bind(amcp_endpoint);
            amcp_acceptor_.listen();
            do_accept_amcp();
            CASPAR_LOG(info) << L"WebSocket AMCP server listening on port " << amcp_port_;

            // Start Monitor server only if monitor_port is not 0
            if (monitor_port_ != 0) {
                tcp::endpoint monitor_endpoint{tcp::v4(), monitor_port_};
                monitor_acceptor_.open(monitor_endpoint.protocol());
                monitor_acceptor_.set_option(net::socket_base::reuse_address(true));
                monitor_acceptor_.bind(monitor_endpoint);
                monitor_acceptor_.listen();
                do_accept_monitor();
                CASPAR_LOG(info) << L"WebSocket Monitor server listening on port " << monitor_port_;
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"Failed to start WebSocket servers: " << u16(e.what());
            throw;
        }
    }

    void stop()
    {
        try {
            amcp_acceptor_.close();
            if (monitor_port_ != 0) {
                monitor_acceptor_.close();
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"Error stopping WebSocket servers: " << u16(e.what());
        }
    }

    void do_accept_amcp()
    {
        amcp_acceptor_.async_accept([self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = spl::make_shared<websocket_amcp_session>(std::move(socket));

                // Apply lifecycle factories
                for (const auto& factory : self->lifecycle_factories_) {
                    auto lifecycle_obj = factory(u8(session->address()));
                    session->add_lifecycle_bound_object(lifecycle_obj.first, lifecycle_obj.second);
                }

                CASPAR_LOG(info) << L"WebSocket AMCP client connected: " << session->address();
                session->start(self->amcp_protocol_factory_);
            }

            if (self->amcp_acceptor_.is_open()) {
                self->do_accept_amcp();
            }
        });
    }

    void do_accept_monitor()
    {
        monitor_acceptor_.async_accept([self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = spl::make_shared<websocket_monitor_session>(std::move(socket));

                {
                    std::lock_guard<std::mutex> lock(self->sessions_mutex_);
                    self->monitor_sessions_.insert(session);
                }

                session->start();
            }

            if (self->monitor_acceptor_.is_open()) {
                self->do_accept_monitor();
            }
        });
    }

    void send_monitor_data(const core::monitor::state& state)
    {
        if (monitor_sessions_.empty()) {
            return;
        }

        try {
            std::string json_data = monitor_state_to_json(state);

            std::lock_guard<std::mutex> lock(sessions_mutex_);

            // Send to all connected monitor clients and remove closed ones
            auto it = monitor_sessions_.begin();
            while (it != monitor_sessions_.end()) {
                if ((*it)->is_open()) {
                    (*it)->send_monitor_data(json_data);
                    ++it;
                } else {
                    it = monitor_sessions_.erase(it);
                }
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"Failed to convert monitor state to JSON: " << u16(e.what());
        }
    }

    void add_client_lifecycle_object_factory(
        const std::function<std::pair<std::wstring, std::shared_ptr<void>>(const std::string&)>& factory)
    {
        lifecycle_factories_.push_back(factory);
    }
};

websocket_server::websocket_server(std::shared_ptr<boost::asio::io_context>       context,
                                   const protocol_strategy_factory<wchar_t>::ptr& amcp_protocol_factory,
                                   uint16_t                                       amcp_port,
                                   uint16_t                                       monitor_port)
    : impl_(spl::make_shared<impl>(context, amcp_protocol_factory, amcp_port, monitor_port))
{
}

websocket_server::websocket_server(std::shared_ptr<boost::asio::io_context>       context,
                                   const protocol_strategy_factory<wchar_t>::ptr& amcp_protocol_factory,
                                   uint16_t                                       amcp_port)
    : impl_(spl::make_shared<impl>(context, amcp_protocol_factory, amcp_port, 0))
{
}

websocket_server::~websocket_server()
{
    if (impl_.get()) {
        impl_->stop();
    }
}

void websocket_server::start() { impl_->start(); }

void websocket_server::stop() { impl_->stop(); }

void websocket_server::send_monitor_data(const core::monitor::state& state) { impl_->send_monitor_data(state); }

void websocket_server::add_client_lifecycle_object_factory(
    const std::function<std::pair<std::wstring, std::shared_ptr<void>>(const std::string& address)>& factory)
{
    impl_->add_client_lifecycle_object_factory(factory);
}

}} // namespace caspar::IO