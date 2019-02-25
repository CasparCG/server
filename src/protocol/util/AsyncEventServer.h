/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

// AsyncEventServer.h: interface for the AsyncServer class.
//////////////////////////////////////////////////////////////////////
#pragma once

#include "protocol_strategy.h"

#include <common/memory.h>

#include <boost/asio.hpp>

namespace caspar { namespace IO {

using lifecycle_factory_t =
    std::function<std::pair<std::wstring, std::shared_ptr<void>>(const std::string& ipv4_address)>;

class AsyncEventServer
{
  public:
    explicit AsyncEventServer(std::shared_ptr<boost::asio::io_service>    service,
                              const protocol_strategy_factory<char>::ptr& protocol,
                              unsigned short                              port);
    ~AsyncEventServer();

    void add_client_lifecycle_object_factory(const lifecycle_factory_t& lifecycle_factory);

    struct implementation;

  private:
    spl::shared_ptr<implementation> impl_;

    AsyncEventServer(const AsyncEventServer&) = delete;
    AsyncEventServer& operator=(const AsyncEventServer&) = delete;
};

}} // namespace caspar::IO
