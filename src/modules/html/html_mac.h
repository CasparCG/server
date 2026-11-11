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

#pragma once

// macOS specifics for the CEF integration.
//
// On macOS CEF must run on the main thread (which runs the Cocoa event loop, see
// shell/macos_main.mm) using an external message pump, and libcef is a framework that has to be
// loaded at runtime. The browser process lives in CasparCG.app/Contents/MacOS and the
// sub-processes in CasparCG.app/Contents/Frameworks/casparcg Helper*.app.

#ifdef __APPLE__

#include <cstdint>
#include <functional>
#include <string>

namespace caspar::html::mac {

// Load "Chromium Embedded Framework.framework" relative to the running executable (works for both
// the main app bundle and the helper bundles). Returns false if it cannot be found.
bool load_cef_library();

// Path of the main helper executable ("casparcg Helper"), empty if not inside an app bundle.
std::string helper_executable_path();

// Path of the main app bundle (CasparCG.app), empty if not inside an app bundle.
std::string main_bundle_path();

// Run synchronously on the main thread.
void run_on_main_thread(const std::function<void()>& func);

// Called from CefBrowserProcessHandler::OnScheduleMessagePumpWork.
void schedule_message_pump_work(int64_t delay_ms);

// Stop pumping (before CefShutdown).
void stop_message_pump();

} // namespace caspar::html::mac

#endif
