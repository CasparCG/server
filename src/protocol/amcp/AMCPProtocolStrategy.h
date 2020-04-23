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
 * Author: Nicklas P Andersson
 */

#pragma once

#include "../util/ProtocolStrategy.h"

#include <common/memory.h>

#include <future>
#include <string>

namespace caspar { namespace protocol { namespace amcp {

class AMCPProtocolStrategy : public IO::IProtocolStrategy
{
  public:
    AMCPProtocolStrategy(const std::wstring& name, const spl::shared_ptr<class amcp_command_repository>& repo);

    virtual ~AMCPProtocolStrategy();

    void        Parse(const std::wstring& msg, IO::ClientInfoPtr pClientInfo) override;
    std::string GetCodepage() const override { return "UTF-8"; }

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;

    AMCPProtocolStrategy(const AMCPProtocolStrategy&) = delete;
    AMCPProtocolStrategy& operator=(const AMCPProtocolStrategy&) = delete;
};

}}} // namespace caspar::protocol::amcp
