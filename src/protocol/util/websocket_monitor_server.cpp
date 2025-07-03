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
    net::io_context&                          ioc_;
    tcp::acceptor                             acceptor_;
    std::shared_ptr<websocket_monitor_client> monitor_client_;
    std::atomic<bool>                         running_{false};

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
            std::make_shared<websocket_monitor_session>(ioc_, std::move(socket), monitor_client_)->run();
        }

        // Accept another connection
        do_accept();
    }
};

struct websocket_monitor_server::impl
{
    std::shared_ptr<boost::asio::io_service>    service_;
    std::shared_ptr<websocket_monitor_client>   monitor_client_;
    uint16_t                                    port_;
    std::shared_ptr<websocket_monitor_listener> listener_;
    std::atomic<bool>                           running_{false};

    impl(std::shared_ptr<boost::asio::io_service>  service,
         std::shared_ptr<websocket_monitor_client> monitor_client,
         uint16_t                                  port)
        : service_(std::move(service))
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
            listener_ = std::make_shared<websocket_monitor_listener>(*service_, endpoint, monitor_client_);
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

websocket_monitor_server::websocket_monitor_server(std::shared_ptr<boost::asio::io_service>  service,
                                                   std::shared_ptr<websocket_monitor_client> monitor_client,
                                                   uint16_t                                  port)
    : impl_(spl::make_shared<impl>(std::move(service), std::move(monitor_client), port))
{
}

websocket_monitor_server::~websocket_monitor_server() { impl_->stop(); }

void websocket_monitor_server::start() { impl_->start(); }

void websocket_monitor_server::stop() { impl_->stop(); }

}}} // namespace caspar::protocol::websocket