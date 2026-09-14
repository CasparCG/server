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
#include "omt_util.h"

#include <common/env.h>
#include <common/except.h>
#include <common/log.h>
#include <common/utf.h>

#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

#include <chrono>
#include <memory>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace caspar { namespace omt {

namespace {

#ifdef _WIN32
const wchar_t* LIBRARY_FILENAME = L"libomt.dll";
#else
const char* LIBRARY_FILENAME = "libomt.so";
#endif

void not_installed()
{
#ifdef _WIN32
    CASPAR_THROW_EXCEPTION(
        not_supported() << msg_info(L"libomt not available. Install the Open Media Transport (OMT) runtime "
                                    L"(https://github.com/openmediatransport/libomtnet/releases) to use OMT "
                                    L"sources/consumers."));
#else
    CASPAR_THROW_EXCEPTION(not_supported()
                           << msg_info(L"libomt not available. Upstream doesn't publish a prebuilt Linux binary - "
                                       L"build libomt (https://github.com/openmediatransport/libomt) and libvmx "
                                       L"(https://github.com/openmediatransport/libvmx) from source and install "
                                       L"libomt.so where the system linker can find it, to use OMT sources/"
                                       L"consumers."));
#endif
}

void not_compatible()
{
    CASPAR_THROW_EXCEPTION(
        not_supported() << msg_info(L"Failed to resolve one or more functions in libomt. The installed OMT "
                                    L"runtime may be incompatible with this version of CasparCG."));
}

template <typename Fn>
bool resolve(void* module, const char* name, Fn& out)
{
#ifdef _WIN32
    out = reinterpret_cast<Fn>(GetProcAddress(reinterpret_cast<HMODULE>(module), name));
#else
    out = reinterpret_cast<Fn>(dlsym(module, name));
#endif
    return out != nullptr;
}

void* open_library()
{
#ifdef _WIN32
    HMODULE module = LoadLibraryW(LIBRARY_FILENAME);
    if (!module) {
        auto dll_path = boost::filesystem::path(env::initial_folder()) / LIBRARY_FILENAME;
        module         = LoadLibraryW(dll_path.c_str());
    }
    if (!module)
        not_installed();

    CASPAR_LOG(info) << L"Loaded " << LIBRARY_FILENAME;
    static std::shared_ptr<void> keep_alive(module, FreeLibrary);
    return module;
#else
    void* handle = dlopen(LIBRARY_FILENAME, RTLD_LOCAL | RTLD_LAZY);
    if (!handle) {
        auto dll_path = boost::filesystem::path(env::initial_folder()) / LIBRARY_FILENAME;
        handle         = dlopen(dll_path.c_str(), RTLD_LOCAL | RTLD_LAZY);
    }
    if (!handle)
        not_installed();

    CASPAR_LOG(info) << L"Loaded " << u16(LIBRARY_FILENAME);
    static std::shared_ptr<void> keep_alive(handle, dlclose);
    return handle;
#endif
}

} // namespace

omt_lib* load_library()
{
    static omt_lib* lib = []() -> omt_lib* {
        void* module = open_library();

        auto result = std::make_unique<omt_lib>();

        bool ok = true;
        ok &= resolve(module, "omt_discovery_getaddresses", result->discovery_getaddresses);
        ok &= resolve(module, "omt_receive_create", result->receive_create);
        ok &= resolve(module, "omt_receive_destroy", result->receive_destroy);
        ok &= resolve(module, "omt_receive", result->receive);
        ok &= resolve(module, "omt_send_create", result->send_create);
        ok &= resolve(module, "omt_send_destroy", result->send_destroy);
        ok &= resolve(module, "omt_send", result->send);
        ok &= resolve(module, "omt_send_getaddress", result->send_getaddress);
        ok &= resolve(module, "omt_send_connections", result->send_connections);

        if (!ok)
            not_compatible();

        return result.release();
    }();

    return lib;
}

std::unique_lock<std::mutex> serialize_create_call()
{
    static std::mutex                    mutex;
    static std::chrono::steady_clock::time_point last_call;

    std::unique_lock<std::mutex> lock(mutex);

    constexpr auto min_gap = std::chrono::milliseconds(300);
    auto           now     = std::chrono::steady_clock::now();
    if (last_call != std::chrono::steady_clock::time_point{} && now - last_call < min_gap)
        std::this_thread::sleep_for(min_gap - (now - last_call));
    last_call = std::chrono::steady_clock::now();

    return lock;
}

std::wstring make_ascii_safe_name(const std::wstring& name)
{
    auto is_ascii = [](const std::wstring& s) {
        for (wchar_t c : s) {
            if (static_cast<unsigned int>(c) > 127)
                return false;
        }
        return true;
    };

    if (is_ascii(name))
        return name;

    std::wstring result = name;

    try {
        // Own locale with the "convert" category normalize() needs (the process' global
        // locale only has "codepage").
        static const boost::locale::generator gen;
        static const std::locale              loc = gen("en_US.UTF-8");

        // Decompose accented letters into base letter + combining mark, then drop the mark.
        std::wstring decomposed = boost::locale::normalize(name, boost::locale::norm_nfd, loc);

        result.clear();
        result.reserve(decomposed.size());
        for (wchar_t c : decomposed) {
            if (c >= 0x0300 && c <= 0x036F)
                continue; // Combining diacritical mark.
            result.push_back(c);
        }
    } catch (...) {
        result = name; // Fall through to the replacement loop below.
    }

    for (wchar_t& c : result) {
        if (static_cast<unsigned int>(c) > 127)
            c = L'_';
    }

    return result;
}

std::vector<std::string> get_current_sources()
{
    std::vector<std::string> result;

    int    count     = 0;
    char** addresses = load_library()->discovery_getaddresses(&count);

    for (int n = 0; n < count; ++n) {
        if (addresses && addresses[n]) {
            result.emplace_back(addresses[n]);
        }
    }

    return result;
}

std::wstring list_command(protocol::amcp::command_context& /*ctx*/)
{
    auto sources = get_current_sources();

    std::wstringstream reply;
    reply << L"200 OMT LIST OK\r\n";
    for (size_t n = 0; n < sources.size(); ++n) {
        reply << (n + 1) << L" \"" << sources[n].c_str() << L"\"\r\n";
    }
    reply << L"\r\n";
    return reply.str();
}

}} // namespace caspar::omt
