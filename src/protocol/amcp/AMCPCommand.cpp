/*
 * Copyright (c) 2018 Norsk rikskringkasting AS
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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "../StdAfx.h"

#include "AMCPCommand.h"
#include <core/producer/stage.h>

namespace caspar { namespace protocol { namespace amcp {

std::future<std::wstring> AMCPCommand::Execute(const spl::shared_ptr<std::vector<channel_context>>& channels)
{
    return command_(ctx_, channels);
}

std::wstring AMCPGroupCommand::name() const
{
    if (commands_.size() == 1) {
        return commands_.at(0)->name();
    }

    return L"BATCH";
}

}}} // namespace caspar::protocol::amcp
