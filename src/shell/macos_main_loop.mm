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
 */

// macOS-specific main loop helpers.
//
// Cocoa requires all window/event handling on the main thread, and Grand
// Central Dispatch blocks enqueued on the main queue (dispatch_async/_sync to
// dispatch_get_main_queue()) only run while the main thread pumps its run loop.
// The Vulkan screen consumer marshals all GLFW/Cocoa calls to the main thread
// via the main queue, so the main thread must process events. We do not call
// [NSApplication run]; instead the server's main thread calls
// macos_process_events() in a loop while ASIO runs on a background thread.

#import <Cocoa/Cocoa.h>
#import <dispatch/dispatch.h>

#ifdef ENABLE_HTML
// CEF requires the host NSApplication to conform to CefAppProtocol when its
// browser-process message loop is pumped on the main thread.
#include "include/cef_application_mac.h"
#endif

// NSApplication subclass that tracks |isHandlingSendEvent|, which CEF requires
// on macOS (CefAppProtocol). Without it CEF aborts when initialized on the main
// thread. The methods are harmless when the HTML/CEF module is not built.
#ifdef ENABLE_HTML
@interface CasparApplication : NSApplication <CefAppProtocol>
#else
@interface CasparApplication : NSApplication
#endif
{
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
#ifdef ENABLE_HTML
    CefScopedSendingEvent sendingEventScoper;
#endif
    [super sendEvent:event];
}
@end

static NSApplication* sharedApp = nil;

// Defined in the html (CEF) module; weak so the shell still links when the HTML
// module is disabled (then null and skipped). On macOS CEF runs on the main
// thread, so it is pumped here as part of main-thread event processing - this
// keeps the CEF-specific workaround out of the shared shell main loop.
extern "C" void caspar_html_tick(void) __attribute__((weak));

extern "C" {

void macos_init_app()
{
    @autoreleasepool {
        if (sharedApp == nil) {
            // Instantiate our CefAppProtocol-conforming NSApplication as the
            // shared instance before anything else touches +sharedApplication.
            sharedApp = [CasparApplication sharedApplication];
            [sharedApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
        }
    }
}

void macos_process_events(double timeout_seconds)
{
    @autoreleasepool {
        if (sharedApp == nil) {
            macos_init_app();
        }

        NSDate* limitDate = [NSDate dateWithTimeIntervalSinceNow:timeout_seconds];

        while (true) {
            NSEvent* event = [sharedApp nextEventMatchingMask:NSEventMaskAny
                                                    untilDate:limitDate
                                                       inMode:NSDefaultRunLoopMode
                                                      dequeue:YES];
            if (event == nil) {
                break;
            }
            [sharedApp sendEvent:event];
            [sharedApp updateWindows];
        }

        // Run the run loop briefly to ensure GCD main-queue blocks are processed.
        [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];

        // Pump CEF on the main thread (no-op when the HTML module is absent).
        if (caspar_html_tick)
            caspar_html_tick();
    }
}

} // extern "C"
