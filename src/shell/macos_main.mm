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
#include <algorithm>
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

// Console window, used when the app is started from Finder (no terminal): shows the server's
// stdout / stderr and sends typed commands to its stdin, like the console window on Windows.
@interface CasparConsole : NSObject <NSWindowDelegate, NSTextFieldDelegate>
@property(strong) NSWindow*            window;
@property(strong) NSTextView*          output;
@property(strong) NSTextField*         input;
@property(strong) NSMutableArray*      history;
@property NSInteger                    historyIndex;
@property(strong) NSMutableData*       pending;  // bytes read, not yet shown (guarded by @synchronized)
@property(strong) NSMutableData*       partial;  // incomplete UTF-8 sequence at the end of a read
@property int                          stdinWrite;
@end

static constexpr NSUInteger max_console_chars = 2'000'000;

@implementation CasparConsole
- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }
    _history = [NSMutableArray array];
    _pending = [NSMutableData data];
    _partial = [NSMutableData data];

    // stdout + stderr -> pipe -> window, input field -> pipe -> stdin
    int out[2], in[2];
    if (pipe(out) != 0 || pipe(in) != 0) {
        return nil;
    }
    dup2(out[1], STDOUT_FILENO);
    dup2(out[1], STDERR_FILENO);
    close(out[1]);
    dup2(in[0], STDIN_FILENO);
    close(in[0]);
    _stdinWrite = in[1];
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    [self createWindow];

    // Read on a background thread and show the output in batches (at most ~30 updates per second),
    // so that heavy logging can't flood the main thread, which also drives the screen consumer and CEF.
    const int readFd = out[0];
    std::thread([self, readFd] {
        char buffer[65536];
        while (true) {
            const ssize_t n = read(readFd, buffer, sizeof(buffer));
            if (n <= 0) {
                break;
            }
            bool schedule;
            @synchronized(self) {
                schedule = self.pending.length == 0;
                [self.pending appendBytes:buffer length:n];
            }
            if (schedule) {
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
                  [self flush];
                });
            }
        }
    }).detach();
    return self;
}

- (void)createWindow
{
    NSRect frame = NSMakeRect(0, 0, 1000, 600);
    _window      = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                            NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    _window.title                = @"CasparCG Server";
    _window.releasedWhenClosed   = NO;
    _window.delegate             = self;
    _window.frameAutosaveName    = @"CasparCGConsole";
    NSView* content              = _window.contentView;
    const CGFloat inputHeight    = 24;
    const CGFloat margin         = 6;

    NSScrollView* scroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(0, inputHeight + 2 * margin, frame.size.width,
                                 frame.size.height - inputHeight - 2 * margin)];
    scroll.hasVerticalScroller = YES;
    scroll.autoresizingMask    = NSViewWidthSizable | NSViewHeightSizable;
    _output                    = [[NSTextView alloc] initWithFrame:scroll.bounds];
    _output.editable           = NO;
    _output.richText           = NO;
    _output.font               = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    _output.textColor          = NSColor.textColor;
    _output.backgroundColor    = NSColor.textBackgroundColor;
    _output.autoresizingMask   = NSViewWidthSizable;
    _output.layoutManager.allowsNonContiguousLayout = YES;
    scroll.documentView                             = _output;
    [content addSubview:scroll];

    _input = [[NSTextField alloc]
        initWithFrame:NSMakeRect(margin, margin, frame.size.width - 2 * margin, inputHeight)];
    _input.font              = [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    _input.placeholderString = @"AMCP command (e.g. INFO, PLAY 1-10 AMB LOOP), q to quit";
    _input.autoresizingMask  = NSViewWidthSizable;
    _input.target            = self;
    _input.action            = @selector(send:);
    _input.delegate          = self;
    [content addSubview:_input];

    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [_window makeFirstResponder:_input];
}

- (void)flush
{
    NSData* data;
    @synchronized(self) {
        data          = [self.pending copy];
        self.pending.length = 0;
    }
    [self.partial appendData:data];

    // Only decode complete UTF-8 sequences, keep a trailing partial one for the next read
    const uint8_t* bytes  = static_cast<const uint8_t*>(self.partial.bytes);
    NSUInteger     length = self.partial.length;
    NSUInteger     cut    = length;
    for (NSUInteger back = 1; back <= 3 && back <= length; ++back) {
        const uint8_t c = bytes[length - back];
        if ((c & 0xC0) == 0x80) {
            continue; // continuation byte
        }
        if (c >= 0xC0) {
            const NSUInteger need = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : 2;
            if (back < need) {
                cut = length - back;
            }
        }
        break;
    }
    NSString* text = [[NSString alloc] initWithBytes:bytes length:cut encoding:NSUTF8StringEncoding];
    if (!text) {
        text = [[NSString alloc] initWithBytes:bytes length:cut encoding:NSISOLatin1StringEncoding];
    }
    [self.partial replaceBytesInRange:NSMakeRange(0, cut) withBytes:nullptr length:0];
    [self append:text];
}

- (void)append:(NSString*)text
{
    if (text.length == 0) {
        return;
    }
    NSScrollView* scroll   = self.output.enclosingScrollView;
    const BOOL    atBottom = NSMaxY(scroll.documentVisibleRect) >= NSMaxY(self.output.bounds) - 20;

    NSTextStorage* storage = self.output.textStorage;
    [storage beginEditing];
    [storage appendAttributedString:[[NSAttributedString alloc]
                                        initWithString:text
                                            attributes:@{
                                                NSFontAttributeName : self.output.font,
                                                NSForegroundColorAttributeName : NSColor.textColor
                                            }]];
    if (storage.length > max_console_chars) {
        // Drop the oldest output, at a line boundary
        NSUInteger drop = storage.length - max_console_chars * 3 / 4;
        drop            = NSMaxRange([storage.string lineRangeForRange:NSMakeRange(drop, 0)]);
        [storage deleteCharactersInRange:NSMakeRange(0, drop)];
    }
    [storage endEditing];

    if (atBottom) {
        [self.output scrollRangeToVisible:NSMakeRange(storage.length, 0)];
    }
}

- (void)send:(id)sender
{
    NSString* line = self.input.stringValue;
    if (line.length == 0) {
        return;
    }
    if (self.history.count == 0 || ![self.history.lastObject isEqualToString:line]) {
        [self.history addObject:line];
    }
    self.historyIndex      = self.history.count;
    self.input.stringValue = @"";

    [self append:[NSString stringWithFormat:@"> %@\n", line]];
    NSData* data = [[line stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding];
    if (write(self.stdinWrite, data.bytes, data.length) < 0) {
        NSBeep();
    }
}

// Up / down arrows: command history
- (BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)command
{
    if (command == @selector(moveUp:) || command == @selector(moveDown:)) {
        if (self.history.count == 0) {
            return YES;
        }
        NSInteger index   = self.historyIndex + (command == @selector(moveUp:) ? -1 : 1);
        index             = std::max<NSInteger>(0, std::min<NSInteger>(index, self.history.count));
        self.historyIndex = index;
        self.input.stringValue = index < (NSInteger)self.history.count ? self.history[index] : @"";
        return YES;
    }
    return NO;
}

- (void)show
{
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.input];
}
@end

static CasparConsole* console     = nil;
static bool           server_done = false;

@interface CasparAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation CasparAppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    if (server_done) {
        return NSTerminateNow;
    }
    // Quit from the menu / Dock: shut the server down cleanly (same as Ctrl+C); the process exits
    // when caspar_main returns.
    std::raise(SIGTERM);
    return NSTerminateCancel;
}

// Click on the Dock icon: bring back the console window if it was closed
- (BOOL)applicationShouldHandleReopen:(NSApplication*)sender hasVisibleWindows:(BOOL)visible
{
    [console show];
    return YES;
}

- (void)showConsole:(id)sender
{
    [console show];
}
@end

static void create_menu(id delegate)
{
    NSMenu*     bar     = [[NSMenu alloc] init];
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [bar addItem:appItem];
    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Hide CasparCG" action:@selector(hide:) keyEquivalent:@"h"];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit CasparCG" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;

    // Edit menu, so that copy / paste / select all work in the console
    NSMenuItem* editItem = [[NSMenuItem alloc] init];
    [bar addItem:editItem];
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
    [editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
    editItem.submenu = editMenu;

    if (console) {
        NSMenuItem* windowItem = [[NSMenuItem alloc] init];
        [bar addItem:windowItem];
        NSMenu*     windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        NSMenuItem* show       = [windowMenu addItemWithTitle:@"Console" action:@selector(showConsole:) keyEquivalent:@"l"];
        show.target            = delegate;
        [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
        windowItem.submenu = windowMenu;
        NSApp.windowsMenu  = windowMenu;
    }
    NSApp.mainMenu = bar;
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

        NSString*  workdir     = select_working_directory(argc, argv);
        const bool interactive = isatty(STDIN_FILENO);
        if (!interactive) {
            // Started from Finder: no terminal, so show our own console window
            console = [[CasparConsole alloc] init];
            if (workdir) {
                [console append:[NSString stringWithFormat:@"Working directory: %@\n\n", workdir]];
            }
        }
        create_menu(delegate);

        std::thread([argc, argv, workdir, interactive] {
            const int code = caspar_main(argc, argv);
            dispatch_async(dispatch_get_main_queue(), ^{
              // 0 = quit, 5 = restart requested (handled by run.sh in a terminal); anything else is
              // an error, which nobody would see when started from Finder.
              if (!interactive && code == 5) {
                  // RESTART command: start a new instance of the app
                  NSWorkspaceOpenConfiguration* config = [NSWorkspaceOpenConfiguration configuration];
                  config.createsNewApplicationInstance = YES;
                  [[NSWorkspace sharedWorkspace]
                      openApplicationAtURL:[NSBundle mainBundle].bundleURL
                             configuration:config
                         completionHandler:^(NSRunningApplication*, NSError*) {
                           dispatch_async(dispatch_get_main_queue(), ^{
                             std::exit(code);
                           });
                         }];
                  return;
              }
              if (console && code != 0) {
                  // Keep the console open, so the error can be read; the app quits with Cmd+Q
                  server_done = true;
                  [console flush];
                  [console append:[NSString stringWithFormat:@"\n*** CasparCG Server stopped with an error "
                                                             @"(exit code %d). Press Cmd+Q to quit. ***\n",
                                                             code]];
                  [console show];
                  return;
              }
              if (!interactive && code != 0) {
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
