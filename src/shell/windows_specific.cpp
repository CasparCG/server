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
 * Author: Robert Nagy, ronag89@gmail.com
 */
#include "platform_specific.h"

#include <common/env.h>
#include <common/log.h>
#include <common/os/windows/windows.h>

#include <atlbase.h>
#include <mmsystem.h>
#include <winnt.h>

#include <cstdlib>
#include <sstream>
#include <thread>

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

extern "C" {
// Force discrete nVidia GPU
// (http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf)
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
// Force discrete AMD GPU (https://community.amd.com/thread/169965 /
// https://gpuopen.com/amdpowerxpressrequesthighperformance/)
_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

namespace caspar {

LONG WINAPI UserUnhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
    try {
        CASPAR_LOG(fatal) << L"#######################\n UNHANDLED EXCEPTION: \n"
                          << L"Adress:" << info->ExceptionRecord->ExceptionAddress << L"\n"
                          << L"Code:" << info->ExceptionRecord->ExceptionCode << L"\n"
                          << L"Flag:" << info->ExceptionRecord->ExceptionFlags << L"\n"
                          << L"Info:" << info->ExceptionRecord->ExceptionInformation << L"\n"
                          << L"Continuing execution. \n#######################";

        CASPAR_LOG_CURRENT_CALL_STACK();
    } catch (...) {
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void setup_prerequisites()
{
    SetUnhandledExceptionFilter(UserUnhandledExceptionFilter);

    // Increase time precision. This will increase accuracy of function like Sleep(1) from 10 ms to 1 ms.
    static struct inc_prec
    {
        inc_prec() { timeBeginPeriod(1); }
        ~inc_prec() { timeEndPeriod(1); }
    } inc_prec;
}

void change_icon(const HICON hNewIcon)
{
    auto hMod              = ::LoadLibrary(L"Kernel32.dll");
    using SCI              = DWORD(__stdcall*)(HICON);
    auto pfnSetConsoleIcon = reinterpret_cast<SCI>(::GetProcAddress(hMod, "SetConsoleIcon"));
    pfnSetConsoleIcon(hNewIcon);
    ::FreeLibrary(hMod);
}

void setup_console_window()
{
    auto  hOut           = GetStdHandle(STD_OUTPUT_HANDLE);
    auto  hIn            = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwPreviousMode = 0;

    if (hIn != INVALID_HANDLE_VALUE && GetConsoleMode(hIn, &dwPreviousMode)) {
        dwPreviousMode &= ~ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS; // disable quick edit mode
        dwPreviousMode &= ENABLE_PROCESSED_INPUT | ~ENABLE_MOUSE_INPUT;    // allow mouse wheel scrolling
        SetConsoleMode(hIn, dwPreviousMode);
    }

    // Disable close button in console to avoid shutdown without cleanup.
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
    DrawMenuBar(GetConsoleWindow());
    SetConsoleCtrlHandler(nullptr, true);

    if (hOut != INVALID_HANDLE_VALUE) {
        // Configure console size and position.
        auto coord = GetLargestConsoleWindowSize(hOut);
        coord.X /= 2;
        coord.Y *= 10;
        SetConsoleScreenBufferSize(hOut, coord);

        SMALL_RECT DisplayArea = {0, 0, 0, 0};
        DisplayArea.Right      = coord.X - 1;
        DisplayArea.Bottom     = (coord.Y / 10 - 1) / 2;
        SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
    }

    change_icon(::LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(101)));

    // Set console title.
    std::wstringstream str;
    str << "CasparCG Server " << env::version() << L" x64 ";
#ifdef COMPILE_RELEASE
    str << " Release";
#elif COMPILE_PROFILE
    str << " Profile";
#elif COMPILE_DEVELOP
    str << " Develop";
#elif COMPILE_DEBUG
    str << " Debug";
#endif
    SetConsoleTitle(str.str().c_str());
}

void increase_process_priority() { SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS); }

void wait_for_keypress()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::system("pause");
}

std::shared_ptr<void> setup_debugging_environment()
{
#ifdef _DEBUG
    HANDLE hLogFile = CreateFile(
        L"crt_log.txt", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    std::shared_ptr<void> crt_log(nullptr, [](HANDLE h) { ::CloseHandle(h); });

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, hLogFile);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, hLogFile);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, hLogFile);

    return crt_log;
#else
    return nullptr;
#endif
}

void wait_for_remote_debugging()
{
#ifdef _DEBUG
    MessageBox(nullptr, L"Now is the time to connect for remote debugging...", L"Debug", MB_OK | MB_TOPMOST);
#endif
}

} // namespace caspar
