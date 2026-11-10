/*
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
 */

// macOS entry point.
//
// AppKit only allows windows (screen consumer, diagnostics) to be created and driven from the
// main thread, and requires the main thread to run the Cocoa event loop. The server itself is
// therefore started on a secondary thread, while the main thread runs [NSApp run]. When the
// server returns, the process exits with its return code (5 = restart, see run.sh).

#import <Cocoa/Cocoa.h>

#include <cstdlib>
#include <thread>

int caspar_main(int argc, char** argv);

int main(int argc, char** argv)
{
    @autoreleasepool {
        [NSApplication sharedApplication];
        // No Dock icon / menu bar, but windows can still be shown.
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        std::thread([argc, argv] {
            const int code = caspar_main(argc, argv);
            dispatch_async(dispatch_get_main_queue(), ^{
              std::exit(code);
            });
        }).detach();

        [NSApp run];
    }
    return 0;
}
