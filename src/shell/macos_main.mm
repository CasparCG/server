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

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include <unistd.h>

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

@interface CasparAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation CasparAppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    // Quit from the menu / Dock: shut the server down cleanly (same as Ctrl+C); the process exits
    // when caspar_main returns.
    std::raise(SIGTERM);
    return NSTerminateCancel;
}
@end

static void create_menu()
{
    NSMenu*     bar     = [[NSMenu alloc] init];
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [bar addItem:appItem];
    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Hide CasparCG" action:@selector(hide:) keyEquivalent:@"h"];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit CasparCG" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;
    NSApp.mainMenu  = bar;
}

static bool has_config_argument(int argc, char** argv)
{
    for (int n = 1; n < argc; ++n) {
        // Finder used to pass -psn_..., ignore any option
        if (argv[n][0] != '-') {
            return true;
        }
    }
    return false;
}

// Started from Finder the working directory is "/", where there is no casparcg.config (and the
// relative media/, log/ ... paths can't be created). Pick a working directory:
//   1. the current directory, if it has a casparcg.config (started from a terminal)
//   2. the directory containing CasparCG.app, if it has a casparcg.config
//   3. ~/Library/Application Support/CasparCG, created with the default configuration
static NSString* select_working_directory(int argc, char** argv)
{
    NSFileManager* fm = [NSFileManager defaultManager];
    if (has_config_argument(argc, argv) || [fm fileExistsAtPath:@"casparcg.config"]) {
        return nil;
    }

    NSString* beside = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
    if ([fm fileExistsAtPath:[beside stringByAppendingPathComponent:@"casparcg.config"]]) {
        [fm changeCurrentDirectoryPath:beside];
        return beside;
    }

    NSString* support = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)
                             .firstObject stringByAppendingPathComponent:@"CasparCG"];
    for (NSString* sub in @[ @"", @"media", @"template", @"data", @"log" ]) {
        [fm createDirectoryAtPath:[support stringByAppendingPathComponent:sub]
            withIntermediateDirectories:YES
                             attributes:nil
                                  error:nil];
    }
    NSString* config   = [support stringByAppendingPathComponent:@"casparcg.config"];
    NSString* defaults = [[NSBundle mainBundle] pathForResource:@"casparcg" ofType:@"config"];
    if (![fm fileExistsAtPath:config] && defaults) {
        [fm copyItemAtPath:defaults toPath:config error:nil];
    }
    [fm changeCurrentDirectoryPath:support];
    return support;
}

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
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        static CasparAppDelegate* delegate = [[CasparAppDelegate alloc] init];
        NSApp.delegate                     = delegate;
        create_menu();

        NSString*  workdir     = select_working_directory(argc, argv);
        const bool interactive = isatty(STDIN_FILENO);

        std::thread([argc, argv, workdir, interactive] {
            const int code = caspar_main(argc, argv);
            dispatch_async(dispatch_get_main_queue(), ^{
              // 0 = quit, 5 = restart requested (handled by run.sh); anything else is an error,
              // which nobody would see when started from Finder.
              if (!interactive && code != 0 && code != 5) {
                  NSString* dir = workdir ?: [[NSFileManager defaultManager] currentDirectoryPath];
                  [NSApp activateIgnoringOtherApps:YES];
                  NSAlert* alert       = [[NSAlert alloc] init];
                  alert.messageText    = @"CasparCG Server stopped with an error";
                  alert.informativeText = [NSString
                      stringWithFormat:@"Exit code %d.\n\nConfiguration and logs: %@\n(casparcg.config, log/)",
                                       code,
                                       dir];
                  [alert addButtonWithTitle:@"Show in Finder"];
                  [alert addButtonWithTitle:@"Close"];
                  if ([alert runModal] == NSAlertFirstButtonReturn) {
                      [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:dir]];
                  }
              }
              std::exit(code);
            });
        }).detach();

        [NSApp activateIgnoringOtherApps:YES];

        [NSApp run];
    }
    return 0;
}
