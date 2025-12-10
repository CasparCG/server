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

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <common/memory.h>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

namespace caspar { namespace protocol { namespace websocket {

// Forward declaration of the session class
class websocket_monitor_session;

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