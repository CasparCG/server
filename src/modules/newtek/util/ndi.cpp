/*
 * Copyright 2018
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
 * Author: Krzysztof Zegzula, zegzulakrzysztof@gmail.com
 */

#include "../StdAfx.h"

#include "ndi.h"

#include <memory>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#endif

#include <common/env.h>
#include <common/except.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace newtek { namespace ndi {

const std::wstring& dll_name()
{
    static std::wstring name = u16(NDILIB_LIBRARY_NAME);

    return name;
}

static std::mutex                              find_instance_mutex;
static std::shared_ptr<NDIlib_find_instance_t> find_instance;

NDIlib_v3* load_library()
{
    static NDIlib_v3* ndi_lib = nullptr;

    if (ndi_lib)
        return ndi_lib;

    auto        dll_path    = boost::filesystem::path(env::initial_folder()) / NDILIB_LIBRARY_NAME;
    const char* runtime_dir = getenv(NDILIB_REDIST_FOLDER);

#ifdef _WIN32
    auto module = LoadLibrary(dll_path.c_str());

    if (!module && runtime_dir) {
        dll_path = boost::filesystem::path(runtime_dir) / NDILIB_LIBRARY_NAME;
        module   = LoadLibrary(dll_path.c_str());
    }

    FARPROC NDIlib_v3_load = NULL;
    if (module) {
        CASPAR_LOG(info) << L"Loaded " << dll_path;
        static std::shared_ptr<void> lib(module, FreeLibrary);
        NDIlib_v3_load = GetProcAddress(module, "NDIlib_v3_load");
    }

    if (!NDIlib_v3_load) {
        not_installed();
    }

#else
    // Try to load the library
    void* hNDILib = dlopen(NDILIB_LIBRARY_NAME, RTLD_LOCAL | RTLD_LAZY);

    if (!hNDILib && runtime_dir) {
        dll_path = boost::filesystem::path(runtime_dir) / NDILIB_LIBRARY_NAME;
        hNDILib  = dlopen(dll_path.c_str(), RTLD_LOCAL | RTLD_LAZY);
    }

    // The main NDI entry point for dynamic loading if we got the library
    const NDIlib_v3* (*NDIlib_v3_load)(void) = NULL;
    if (hNDILib) {
        CASPAR_LOG(info) << L"Loaded " << dll_path;
        static std::shared_ptr<void> lib(hNDILib, dlclose);
        *((void**)&NDIlib_v3_load) = dlsym(hNDILib, "NDIlib_v3_load");
    }

    if (!NDIlib_v3_load) {
        not_installed();
    }

#endif

    ndi_lib = (NDIlib_v3*)(NDIlib_v3_load());

    if (!ndi_lib->NDIlib_initialize()) {
        not_initialized();
    }

    find_instance.reset(new NDIlib_find_instance_t(ndi_lib->NDIlib_find_create_v2(nullptr)),
                        [](NDIlib_find_instance_t* p) { ndi_lib->NDIlib_find_destroy(*p); });
    return ndi_lib;
}

std::map<std::string, NDIlib_source_t> get_current_sources()
{
    auto                        sources_map = std::map<std::string, NDIlib_source_t>();
    uint32_t                    no_sources;
    std::lock_guard<std::mutex> guard(find_instance_mutex);
    const NDIlib_source_t*      sources =
        load_library()->NDIlib_find_get_current_sources(*(find_instance.get()), &no_sources);
    for (uint32_t i = 0; i < no_sources; i++) {
        sources_map.emplace(std::string(sources[i].p_ndi_name), sources[i]);
    }
    return sources_map;
}

void not_installed()
{
    CASPAR_THROW_EXCEPTION(not_supported()
                           << msg_info(dll_name() + L" not available. Install NDI Redist version 3.7 or higher from " +
                                       u16(NDILIB_REDIST_URL)));
}

void not_initialized()
{
    CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Unable to initialize NDI on this system."));
}

std::wstring list_command(protocol::amcp::command_context& ctx)
{
    auto ndi_lib = load_library();
    if (!ndi_lib) {
        return L"501 NDI LIST FAILED\r\n";
    }
    std::wstringstream replyString;
    replyString << L"200 NDI LIST OK\r\n";
    uint32_t               no_sources;
    const NDIlib_source_t* sources = ndi_lib->NDIlib_find_get_current_sources(*(find_instance.get()), &no_sources);
    for (uint32_t i = 0; i < no_sources; i++) {
        replyString << i + 1 << L" \"" << sources[i].p_ndi_name << L"\" " << sources[i].p_url_address << L"\r\n";
    }
    replyString << L"\r\n";
    return replyString.str();
}

}}} // namespace caspar::newtek::ndi
