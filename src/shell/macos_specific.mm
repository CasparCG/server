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

#import <Foundation/Foundation.h>

#include <cstdlib>

#include <common/log.h>
#include <exception>

#include <iostream>

namespace caspar {

void setup_process_scheduling()
{
    // Opt out of App Nap and timer coalescing: playout must keep realtime timing even when the
    // process is in the background or the display sleeps.
    static id<NSObject> activity = [[NSProcessInfo processInfo]
        beginActivityWithOptions:NSActivityUserInitiated | NSActivityLatencyCritical
                          reason:@"CasparCG realtime playout"];
    (void)activity;
}

void setup_prerequisites()
{
    // Self-contained CasparCG.app: use the bundled MoltenVK Vulkan driver (and not e.g. one from
    // Homebrew), unless the user explicitly selected drivers.
    if (!std::getenv("VK_DRIVER_FILES") && !std::getenv("VK_ICD_FILENAMES")) {
        NSString* icd = [[NSBundle mainBundle] pathForResource:@"MoltenVK_icd"
                                                        ofType:@"json"
                                                   inDirectory:@"vulkan/icd.d"];
        if (icd) {
            setenv("VK_DRIVER_FILES", icd.UTF8String, 0);
        }
    }

    // Enable utf8 console input and output
    std::wcout.sync_with_stdio(false);
    std::wcout.imbue(std::locale(""));
    std::wcin.imbue(std::locale(""));


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
