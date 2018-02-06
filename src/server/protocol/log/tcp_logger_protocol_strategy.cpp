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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "../StdAfx.h"

#include "tcp_logger_protocol_strategy.h"

#include <common/log.h>

namespace caspar { namespace protocol { namespace log {

class tcp_logger_protocol_strategy : public IO::protocol_strategy<char>
{
    spl::shared_ptr<IO::client_connection<char>> client_connection_;
    std::shared_ptr<void>                        log_sink_ =
        caspar::log::add_preformatted_line_sink([=](std::string line) { handle_log_line(std::move(line)); });

  public:
    tcp_logger_protocol_strategy(spl::shared_ptr<IO::client_connection<char>> client_connection)
        : client_connection_(std::move(client_connection))
    {
    }

    void handle_log_line(std::string line)
    {
        line += "\r\n";
        client_connection_->send(std::move(line));
    }

    void parse(const std::string& data) override {}
};

spl::shared_ptr<IO::protocol_strategy<char>>
tcp_logger_protocol_strategy_factory::create(const spl::shared_ptr<IO::client_connection<char>>& client_connection)
{
    return spl::make_shared<tcp_logger_protocol_strategy>(client_connection);
}

}}} // namespace caspar::protocol::log
