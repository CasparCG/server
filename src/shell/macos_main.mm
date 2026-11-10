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
#include <cstring>
#include <thread>

int caspar_main(int argc, char** argv);

// Chromium (CEF) requires the NSApplication to implement these protocols (see CEF's
// include/cef_application_mac.h). They are declared here to keep the shell independent of CEF;
// the Objective-C runtime matches protocols by name.
@protocol CrAppProtocol
- (BOOL)isHandlingSendEvent;
@end

@protocol CrAppControlProtocol <CrAppProtocol>
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;
@end

@protocol CefAppProtocol <CrAppControlProtocol>
@end

@interface CasparApplication : NSApplication <CefAppProtocol> {
  @private
    BOOL handlingSendEvent_;
}
@end

@implementation CasparApplication
- (BOOL)isHandlingSendEvent
{
    return handlingSendEvent_;
}
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent
{
    handlingSendEvent_ = handlingSendEvent;
}
- (void)sendEvent:(NSEvent*)event
{
    BOOL previous      = handlingSendEvent_;
    handlingSendEvent_ = YES;
    [super sendEvent:event];
    handlingSendEvent_ = previous;
}
@end

int main(int argc, char** argv)
{
    // CEF sub-processes (renderer, GPU, ...) are started from the helper bundles with --type=...
    // They run their own message loop on the main thread. Don't initialize AppKit here: like
    // Chromium's own helpers they must not create an NSApplication (on macOS 26 that starts AppIntents
    // XPC registration whose objects are later freed by Chromium's allocator, crashing the helper).
    for (int n = 1; n < argc; ++n) {
        if (std::strncmp(argv[n], "--type=", 7) == 0) {
            return caspar_main(argc, argv);
        }
    }

    @autoreleasepool {
        [CasparApplication sharedApplication];
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
