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

#include "clk_command_processor.h"

namespace caspar { namespace protocol { namespace CLK {

clk_command_processor& clk_command_processor::add_handler(const std::wstring&        command_name,
                                                          const clk_command_handler& handler)
{
    handlers_.insert(std::make_pair(command_name, handler));

    return *this;
}

bool clk_command_processor::handle(const std::wstring& command_name, const std::vector<std::wstring>& parameters)
{
    auto handler = handlers_.find(command_name);

    if (handler == handlers_.end())
        return false;

    handler->second(parameters);

    return true;
}

}}} // namespace caspar::protocol::CLK
