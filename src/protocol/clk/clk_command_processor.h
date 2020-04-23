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

#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/function.hpp>

namespace caspar { namespace protocol { namespace CLK {

using clk_command_handler = boost::function<void(const std::vector<std::wstring>&)>;

/**
 * CLK command processor composed by multiple command handlers.
 *
 * Not thread-safe.
 */
class clk_command_processor
{
    std::map<std::wstring, clk_command_handler> handlers_;

  public:
    /**
     * Register a handler for a specific command.
     *
     * @param command_name The command name to use this handler for.
     * @param handler      The handler that will handle all commands with the
     *                     specified name.
     *
     * @return this command processor for method chaining.
     */
    clk_command_processor& add_handler(const std::wstring& command_name, const clk_command_handler& handler);

    /**
     * Handle an incoming command.
     *
     * @param command_name The command name.
     * @param parameters   The raw parameters supplied with the command.
     *
     * @return true if the command was handled, false if no handler was
     *         registered to handle the command.
     */
    bool handle(const std::wstring& command_name, const std::vector<std::wstring>& parameters);
};

}}} // namespace caspar::protocol::CLK
