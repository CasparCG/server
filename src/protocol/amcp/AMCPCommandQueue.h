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

#include "AMCPCommand.h"

#include <common/executor.h>
#include <common/future.h>
#include <common/memory.h>

namespace caspar { namespace protocol { namespace amcp {

struct CommandResult
{
    int          status = 0;
    std::wstring message;

    CommandResult(int status, std::wstring message)
        : status(status)
        , message(message)
    {
    }

    static std::future<CommandResult> create(int status, std::wstring message)
    {
        return make_ready_future(CommandResult(status, message));
    }

    std::wstring to_string()
    {
        if (status == 0)
            return message;
        else
            return std::to_wstring(status) + L" " + message;
    }
};

class AMCPCommandQueue
{
  public:
    using ptr_type = spl::shared_ptr<AMCPCommandQueue>;

    AMCPCommandQueue(const std::wstring& name, const spl::shared_ptr<std::vector<channel_context>>& channels);
    ~AMCPCommandQueue();

    std::future<std::vector<CommandResult>> AddCommand(std::shared_ptr<AMCPGroupCommand> command);

  private:
    std::vector<CommandResult> Execute(std::shared_ptr<AMCPGroupCommand> cmd) const;

    executor                                            executor_;
    const spl::shared_ptr<std::vector<channel_context>> channels_;
};

}}} // namespace caspar::protocol::amcp
