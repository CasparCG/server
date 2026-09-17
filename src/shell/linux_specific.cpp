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
#include <locale>

#include <X11/Xlib.h>

namespace caspar {

namespace {
// Prefer a guaranteed Unicode-capable locale that requires no OS locale generation (available on
// any glibc >= 2.35), rather than trusting the deployment environment to have one configured -
// falling back to the environment, and finally to "C" (ASCII-only, but never throws), only if
// that isn't recognized. This runs at startup, so an unhandled exception here would prevent the
// server from starting at all on a misconfigured environment. See GitHub issues #1364, #1018.
std::locale safe_console_locale()
{
    try {
        return std::locale("C.UTF-8");
    } catch (const std::runtime_error&) {
    }
    try {
        return std::locale("");
    } catch (const std::runtime_error&) {
    }
    return std::locale::classic();
}
} // namespace

void setup_process_scheduling()
{
    // Nothing to do, linux does not throttle background processes like this.
}

void setup_prerequisites()
{
    // Enable utf8 console input and output
    std::wcout.sync_with_stdio(false);
    std::wcout.imbue(safe_console_locale());
    std::wcin.imbue(safe_console_locale());

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
