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

    std::future<std::wstring> Execute(const spl::shared_ptr<std::vector<channel_context>>& channels)
    {
        return command_(ctx_, channels);
    }

    const std::vector<std::wstring>& parameters() const { return ctx_.parameters; }

    int channel_index() const { return ctx_.channel_index; }

    const std::wstring& name() const { return name_; }

    const std::wstring& result() const { return result_; }
};

}}} // namespace caspar::protocol::amcp
