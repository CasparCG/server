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
#include <optional>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace caspar { namespace omt {

namespace {

// --- Loading the library ---

#ifdef _WIN32
const wchar_t* const library_filename = L"libomt.dll";

void* open_module(const boost::filesystem::path& path) { return LoadLibraryW(path.c_str()); }

void* find_symbol(void* module, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), name));
}

void close_module(void* module) { FreeLibrary(static_cast<HMODULE>(module)); }
#else
const char* const library_filename = "libomt.so";

void* open_module(const boost::filesystem::path& path) { return dlopen(path.c_str(), RTLD_LOCAL | RTLD_LAZY); }

void* find_symbol(void* module, const char* name) { return dlsym(module, name); }

void close_module(void* module) { dlclose(module); }
#endif

[[noreturn]] void throw_library_not_found()
{
    CASPAR_THROW_EXCEPTION(not_supported() << msg_info(
                               L"libomt was not found. It is built together with CasparCG, unless CMake was "
                               L"configured with -DOMT_BUILD_FROM_SOURCE=OFF. Otherwise libomt "
                               L"(https://github.com/openmediatransport/libomt) has to be installed where the system "
                               L"can find it."));
}

[[noreturn]] void throw_library_incompatible()
{
    CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"libomt is missing one or more functions that CasparCG needs. "
                                                       L"The installed version may not work with this CasparCG."));
}

// Looks for libomt on the system library path first, then in the folder CasparCG was started from.
void* open_library()
{
    void* module = open_module(library_filename);

    if (module == nullptr) {
        const auto path_next_to_server = boost::filesystem::path(env::initial_folder()) / library_filename;
        module                         = open_module(path_next_to_server);
    }

    if (module == nullptr) {
        throw_library_not_found();
    }

    CASPAR_LOG(info) << L"Loaded " << boost::filesystem::path(library_filename).wstring();

    // Unloads the library when CasparCG exits.
    static std::shared_ptr<void> keep_loaded(module, close_module);
    return module;
}

template <typename Function>
bool find_function(void* module, const char* name, Function& function)
{
    function = reinterpret_cast<Function>(find_symbol(module, name));
    return function != nullptr;
}

// --- Names ---

bool is_ascii(wchar_t character) { return static_cast<unsigned int>(character) <= 127; }

bool is_ascii(const std::wstring& text)
{
    for (const wchar_t character : text) {
        if (is_ascii(character) == false) {
            return false;
        }
    }
    return true;
}

bool is_combining_mark(wchar_t character) { return character >= 0x0300 && character <= 0x036F; }

// Splits accented letters into the letter and its combining marks, then drops the marks.
std::wstring remove_diacritics(const std::wstring& text)
{
    // The process-wide locale has no "convert" facet, which normalize() needs.
    static const boost::locale::generator generator;
    static const std::locale              normalization_locale = generator("en_US.UTF-8");

    const std::wstring decomposed = boost::locale::normalize(text, boost::locale::norm_nfd, normalization_locale);

    std::wstring result;
    result.reserve(decomposed.size());
    for (const wchar_t character : decomposed) {
        if (is_combining_mark(character) == false) {
            result.push_back(character);
        }
    }
    return result;
}

} // namespace

omt_lib* load_library()
{
    static omt_lib* const lib = [] {
        void* const module = open_library();

        auto loaded = std::make_unique<omt_lib>();

        bool all_functions_found = true;
        all_functions_found &= find_function(module, "omt_discovery_getaddresses", loaded->discovery_getaddresses);
        all_functions_found &= find_function(module, "omt_receive_create", loaded->receive_create);
        all_functions_found &= find_function(module, "omt_receive_destroy", loaded->receive_destroy);
        all_functions_found &= find_function(module, "omt_receive", loaded->receive);
        all_functions_found &= find_function(module, "omt_send_create", loaded->send_create);
        all_functions_found &= find_function(module, "omt_send_destroy", loaded->send_destroy);
        all_functions_found &= find_function(module, "omt_send", loaded->send);
        all_functions_found &= find_function(module, "omt_send_getaddress", loaded->send_getaddress);
        all_functions_found &= find_function(module, "omt_send_connections", loaded->send_connections);

        if (all_functions_found == false) {
            throw_library_incompatible();
        }

        return loaded.release();
    }();

    return lib;
}

std::unique_lock<std::mutex> serialize_create_call()
{
    static std::mutex                                           mutex;
    static std::optional<std::chrono::steady_clock::time_point> last_call_time;

    constexpr auto minimum_gap = std::chrono::milliseconds(300);

    std::unique_lock<std::mutex> lock(mutex);

    if (last_call_time.has_value() == true) {
        const auto time_since_last_call = std::chrono::steady_clock::now() - last_call_time.value();
        if (time_since_last_call < minimum_gap) {
            std::this_thread::sleep_for(minimum_gap - time_since_last_call);
        }
    }
    last_call_time = std::chrono::steady_clock::now();

    return lock;
}

std::wstring make_ascii_safe_name(const std::wstring& name)
{
    if (is_ascii(name) == true) {
        return name;
    }

    std::wstring result;
    try {
        result = remove_diacritics(name);
    } catch (...) {
        // Without normalization everything non-ASCII is replaced below.
        result = name;
    }

    for (wchar_t& character : result) {
        if (is_ascii(character) == false) {
            character = L'_';
        }
    }
    return result;
}

std::vector<std::string> get_current_sources()
{
    int          address_count = 0;
    char** const addresses     = load_library()->discovery_getaddresses(&address_count);

    std::vector<std::string> sources;
    if (addresses == nullptr) {
        return sources;
    }

    for (int index = 0; index < address_count; ++index) {
        if (addresses[index] != nullptr) {
            sources.emplace_back(addresses[index]);
        }
    }
    return sources;
}

std::wstring list_command(protocol::amcp::command_context& /*ctx*/)
{
    const auto sources = get_current_sources();

    std::wstringstream reply;
    reply << L"200 OMT LIST OK\r\n";
    for (size_t index = 0; index < sources.size(); ++index) {
        reply << (index + 1) << L" \"" << u16(sources[index]) << L"\"\r\n";
    }
    reply << L"\r\n";
    return reply.str();
}

}} // namespace caspar::omt
