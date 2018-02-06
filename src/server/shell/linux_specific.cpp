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

#include "platform_specific.h"

#include <common/log.h>
#include <exception>

#include <iostream>

#include <X11/Xlib.h>

namespace caspar {

void setup_prerequisites()
{
    XInitThreads();

    std::set_terminate([] { CASPAR_LOG_CURRENT_EXCEPTION(); });
}

void setup_console_window()
{
    // TODO: implement.
}

void increase_process_priority()
{
    // TODO: implement.
}

void wait_for_keypress()
{
    // TODO: implement if desirable.
}

std::shared_ptr<void> setup_debugging_environment()
{
    // TODO: implement if applicable.
    return nullptr;
}

void wait_for_remote_debugging()
{
    // TODO: implement if applicable.
}

} // namespace caspar
