/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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
 * Author: Krzysztof Zegzula, zegzulakrzysztof@gmail.com
 */
#pragma once

#include "../interop/Processing.NDI.Lib.h"
#include "protocol/amcp/AMCPCommand.h"
#include <string>

namespace caspar { namespace newtek { namespace ndi {

const std::wstring&                    dll_name();
NDIlib_v3*                             load_library();
std::map<std::string, NDIlib_source_t> get_current_sources();
void                                   not_initialized();
void                                   not_installed();

std::wstring list_command(protocol::amcp::command_context& ctx);

}}} // namespace caspar::newtek::ndi
