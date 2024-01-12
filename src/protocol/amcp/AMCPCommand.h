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

#include "amcp_shared.h"

namespace caspar { namespace protocol { namespace amcp {

class AMCPCommand
{
  private:
    const command_context_simple ctx_;
    const amcp_command_func      command_;
    const std::wstring           name_;
    const std::wstring           result_;

  public:
    AMCPCommand(const command_context_simple& ctx, const amcp_command_func& command, const std::wstring& name)

        : ctx_(ctx)
        , command_(command)
        , name_(name)
    {
    }

    using ptr_type = std::shared_ptr<AMCPCommand>;

    std::future<std::wstring> Execute(const spl::shared_ptr<std::vector<channel_context>>& channels);

    const std::vector<std::wstring>& parameters() const { return ctx_.parameters; }

    int channel_index() const { return ctx_.channel_index; }

    const std::wstring& name() const { return name_; }

    const std::wstring& result() const { return result_; }
};

class AMCPGroupCommand
{
    const std::vector<std::shared_ptr<AMCPCommand>> commands_;
    const bool                                      has_client_;
    const bool                                      is_batch_;

  public:
    AMCPGroupCommand(const std::vector<std::shared_ptr<AMCPCommand>> commands, bool has_client)
        : commands_(commands)
        , has_client_(has_client)
        , is_batch_(true)
    {
    }

    AMCPGroupCommand(const std::vector<std::shared_ptr<AMCPCommand>> commands)
        : commands_(commands)
        , has_client_(false)
        , is_batch_(true)
    {
    }

    AMCPGroupCommand(const std::shared_ptr<AMCPCommand> command)
        : commands_({command})
        , has_client_(false)
        , is_batch_(false)
    {
    }

    bool HasClient() const { return has_client_; }

    std::vector<std::wstring> GetResults()
    {
        std::vector<std::wstring> results;
        results.reserve(commands_.size());

        for (size_t i = 0; i < commands_.size(); i++) {
            results.emplace_back(commands_[i]->result());
        }

        return results;
    }

    std::wstring name() const;

    std::vector<std::shared_ptr<AMCPCommand>> Commands() const { return commands_; }
};

}}} // namespace caspar::protocol::amcp
