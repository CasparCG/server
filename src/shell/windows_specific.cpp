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

#include <fcntl.h>
#include <io.h>

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
    // Enable utf8 console input and output
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    SetUnhandledExceptionFilter(UserUnhandledExceptionFilter);

    // Increase time precision. This will increase accuracy of function like Sleep(1) from 10 ms to 1 ms.
    static struct inc_prec
    {
        inc_prec() { timeBeginPeriod(1); }
        ~inc_prec() { timeEndPeriod(1); }
    } inc_prec;
}

void increase_process_priority() { SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS); }

} // namespace caspar
