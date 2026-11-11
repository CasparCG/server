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

#include "html_mac.h"

#import <Foundation/Foundation.h>

#include <include/cef_app.h>
#include <include/wrapper/cef_library_loader.h>

#include <common/log.h>

#include <mach-o/dyld.h>

#include <atomic>
#include <filesystem>
#include <mutex>

namespace caspar::html::mac {

namespace fs = std::filesystem;

namespace {

const char* const FRAMEWORK_NAME = "Chromium Embedded Framework.framework";
const char* const HELPER_NAME    = "casparcg Helper";

fs::path executable_path()
{
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) {
        return {};
    }
    std::error_code ec;
    auto            path = fs::canonical(fs::path(buf.c_str()), ec);
    return ec ? fs::path(buf.c_str()) : path;
}

// <bundle>/Contents/MacOS/<exe> -> <bundle>/Contents/Frameworks
fs::path frameworks_dir_for(const fs::path& exe)
{
    return exe.parent_path().parent_path() / "Frameworks";
}

std::atomic<bool> g_pump_active{true};
std::atomic<bool> g_in_work{false};

// Safety net recommended by CEF for external message pumps: also do work periodically, so a
// scheduled call that was skipped (re-entrancy) or lost during startup cannot stall CEF.
constexpr int64_t MAX_PUMP_INTERVAL_MS = 1000 / 60;
dispatch_source_t g_pump_timer         = nullptr;

void do_work()
{
    // Must run on the main thread; never re-enter CefDoMessageLoopWork (it may spin nested run
    // loops which drain the main queue). A skipped call is picked up by the periodic timer.
    if (!g_pump_active || g_in_work.exchange(true)) {
        return;
    }
    CefDoMessageLoopWork();
    g_in_work = false;
}

void ensure_pump_timer()
{
    static std::once_flag once;
    std::call_once(once, [] {
        g_pump_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        dispatch_source_set_timer(g_pump_timer,
                                  dispatch_time(DISPATCH_TIME_NOW, MAX_PUMP_INTERVAL_MS * NSEC_PER_MSEC),
                                  MAX_PUMP_INTERVAL_MS * NSEC_PER_MSEC,
                                  NSEC_PER_MSEC);
        dispatch_source_set_event_handler(g_pump_timer, ^{
          do_work();
        });
        dispatch_resume(g_pump_timer);
    });
}

} // namespace

bool load_cef_library()
{
    static std::once_flag once;
    static bool           loaded = false;

    std::call_once(once, [] {
        const auto exe = executable_path();

        // Main app:   CasparCG.app/Contents/MacOS/casparcg
        //               -> CasparCG.app/Contents/Frameworks/<framework>
        // Helper app: CasparCG.app/Contents/Frameworks/casparcg Helper.app/Contents/MacOS/casparcg Helper
        //               -> CasparCG.app/Contents/Frameworks/<framework>
        const fs::path candidates[] = {
            frameworks_dir_for(exe) / FRAMEWORK_NAME,
            exe.parent_path().parent_path().parent_path().parent_path() / FRAMEWORK_NAME,
        };

        for (const auto& framework : candidates) {
            const auto library = framework / "Chromium Embedded Framework";
            if (fs::exists(library)) {
                loaded = cef_load_library(library.c_str()) == 1;
                if (!loaded) {
                    CASPAR_LOG(error) << L"[html] Failed to load " << library.wstring();
                }
                return;
            }
        }
    });

    return loaded;
}

std::string helper_executable_path()
{
    const auto helper = frameworks_dir_for(executable_path()) / (std::string(HELPER_NAME) + ".app") / "Contents" /
                        "MacOS" / HELPER_NAME;
    return fs::exists(helper) ? helper.string() : std::string{};
}

std::string main_bundle_path()
{
    const auto bundle = executable_path().parent_path().parent_path().parent_path();
    return bundle.extension() == ".app" ? bundle.string() : std::string{};
}

void run_on_main_thread(const std::function<void()>& func)
{
    if ([NSThread isMainThread]) {
        func();
    } else {
        dispatch_sync(dispatch_get_main_queue(), ^{
          func();
        });
    }
}

void schedule_message_pump_work(int64_t delay_ms)
{
    if (!g_pump_active) {
        return;
    }
    ensure_pump_timer();
    if (delay_ms <= 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          do_work();
        });
    } else {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay_ms * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
          do_work();
        });
    }
}

void stop_message_pump()
{
    g_pump_active = false;
    if (g_pump_timer) {
        dispatch_source_cancel(g_pump_timer);
    }
}

} // namespace caspar::html::mac
