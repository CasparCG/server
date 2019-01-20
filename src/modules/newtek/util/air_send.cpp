/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../StdAfx.h"

#include "air_send.h"

#include <memory>

#include <Windows.h>

namespace caspar { namespace newtek { namespace airsend {

void* (*create)(const int   width,
                const int   height,
                const int   timescale,
                const int   duration,
                const bool  progressive,
                const float aspect_ratio,
                const bool  audio_enabled,
                const int   num_channels,
                const int   sample_rate)                                         = nullptr;
void (*destroy)(void* instance)                                                = nullptr;
bool (*add_audio)(void* instance, const short* samples, const int num_samples) = nullptr;
bool (*add_frame_bgra)(void* instance, const unsigned char* data)              = nullptr;

const std::wstring& dll_name()
{
    static std::wstring name = L"Processing.AirSend.x64.dll";

    return name;
}

std::shared_ptr<void> load_library()
{
    auto module = LoadLibrary(dll_name().c_str());

    if (!module)
        return nullptr;

    std::shared_ptr<void> lib(module, FreeLibrary);

    wchar_t actualFilename[256];

    GetModuleFileNameW(module, actualFilename, sizeof(actualFilename));

    CASPAR_LOG(info) << L"Loaded " << actualFilename;

    create         = reinterpret_cast<decltype(create)>(GetProcAddress(module, "AirSend_Create"));
    destroy        = reinterpret_cast<decltype(destroy)>(GetProcAddress(module, "AirSend_Destroy"));
    add_audio      = reinterpret_cast<decltype(add_audio)>(GetProcAddress(module, "AirSend_add_audio"));
    add_frame_bgra = reinterpret_cast<decltype(add_frame_bgra)>(GetProcAddress(module, "AirSend_add_frame_bgra"));

    if (create == nullptr || destroy == nullptr || add_audio == nullptr || add_frame_bgra == nullptr) {
        create         = nullptr;
        destroy        = nullptr;
        add_audio      = nullptr;
        add_frame_bgra = nullptr;

        return nullptr;
    }

    return lib;
}

bool is_available()
{
    static std::shared_ptr<void> lib = load_library();

    return static_cast<bool>(lib);
}

}}} // namespace caspar::newtek::airsend
