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

#if defined _DEBUG && defined _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

#include "included_modules.h"
#include "platform_specific.h"
#include "server.h"

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/util/strategy_adapters.h>

#include <common/env.h>
#include <common/except.h>
#include <common/log.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/stacktrace.hpp>

#include <atomic>
#include <future>
#include <thread>

#include <clocale>
#include <csignal>

#include "napi.h"


namespace caspar {

void setup_global_locale()
{
    boost::locale::generator gen;
#if BOOST_VERSION >= 108100
    gen.categories(boost::locale::category_t::codepage);
#else
    gen.categories(boost::locale::codepage_facet);
#endif

    std::locale::global(gen(""));

    // sscanf is used in for example FFmpeg where we want decimals to be parsed as .
    std::setlocale(LC_ALL, "C");
}

void print_info()
{
    CASPAR_LOG(info) << L"############################################################################";
    CASPAR_LOG(info) << L"CasparCG Server is distributed by the Swedish Broadcasting Corporation (SVT)";
    CASPAR_LOG(info) << L"under the GNU General Public License GPLv3 or higher.";
    CASPAR_LOG(info) << L"Please see LICENSE.TXT for details.";
    CASPAR_LOG(info) << L"http://www.casparcg.com/";
    CASPAR_LOG(info) << L"############################################################################";
    CASPAR_LOG(info) << L"Starting CasparCG Video and Graphics Playout Server " << env::version();
}

// void signal_handler(int signum)
// {
//     ::signal(signum, SIG_DFL);
//     boost::stacktrace::safe_dump_to("./backtrace.dump");
//     ::raise(SIGABRT);
// }

// void terminate_handler()
// {
//     try {
//         std::rethrow_exception(std::current_exception());
//     } catch (...) {
//         CASPAR_LOG_CURRENT_EXCEPTION();
//     }
//     std::abort();
// }

} // namespace caspar


void fake_shutdown(bool restart) {
    // Ignore
}

struct CasparCgInstanceData {
    std::unique_ptr<caspar::server> caspar_server;
    // std::shared_ptr<caspar::> amcp;
    std::shared_ptr<caspar::IO::protocol_strategy<wchar_t>> amcp;
};

Napi::Value InitServer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
  if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!").ThrowAsJavaScriptException();
        return env.Null();
  }

  if (instance_data->caspar_server) {
        Napi::Error::New(env, "Server is already running, it must be stopped before you can call init again").ThrowAsJavaScriptException();
        return env.Null();
  }

  using namespace caspar;

    // setup_global_locale();

    setup_prerequisites();

    // Increase process priority.
    increase_process_priority();

    std::wstring config_file_name(L"casparcg.config");

    try {
        log::add_cout_sink();
        env::configure(config_file_name);

        {
            std::wstring target_level = env::properties().get(L"configuration.log-level", L"info");
            if (!log::set_log_level(target_level)) {
                log::set_log_level(L"info");
                std::wcout << L"Failed to set log level [" << target_level << L"]" << std::endl;
            }
        }

    auto shutdown = [=](bool restart) {
        // TODO - this is probably not needed?
     };

    print_info();

    // Create server object which initializes channels, protocols and controllers.
    instance_data->caspar_server.reset(new server(shutdown));

    // For example CEF resets the global locale, so this is to reset it back to "our" preference.
    setup_global_locale();

    std::wstringstream                                      str;
    boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
    boost::property_tree::write_xml(str, env::properties(), w);
    CASPAR_LOG(info) << boost::filesystem::absolute(config_file_name).lexically_normal()
                     << L":\n-----------------------------------------\n"
                     << str.str() << L"-----------------------------------------";

    instance_data->caspar_server->start();

    // Create a dummy client which prints amcp responses to console.
    auto console_client = spl::make_shared<IO::ConsoleClientInfo>();

    instance_data->amcp =(
        protocol::amcp::create_wchar_amcp_strategy_factory(L"Console", instance_data->caspar_server->get_amcp_command_repository())
            ->create(console_client));

    } catch (boost::property_tree::file_parser_error& e) {
        Napi::Error::New(env, "Please check the configuration for errors").ThrowAsJavaScriptException();
    } catch (user_error&) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        Napi::Error::New(env, "Please check the configuration for errors").ThrowAsJavaScriptException();
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        Napi::Error::New(env, "Unhandled exception").ThrowAsJavaScriptException();
    }

    return env.Null();
}

Napi::Value StopServer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
  if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!").ThrowAsJavaScriptException();
        return env.Null();
  }


  if (!instance_data->caspar_server) {
    // Nothing to do
        return env.Null();
  }

  instance_data->amcp.reset();

    instance_data->caspar_server = nullptr;

    return env.Null();
}


Napi::Value ParseCommand(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
  if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!").ThrowAsJavaScriptException();
        return env.Null();
  }

  if (!instance_data->caspar_server || !instance_data->amcp) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
  }

    if (info.Length() < 1) {
        Napi::Error::New(env, "Expected command string").ThrowAsJavaScriptException();
        return env.Null();
    }

      std::string cmd = info[0].As<Napi::String>().Utf8Value();
      instance_data->amcp->parse(caspar::u16(cmd));

    return env.Null();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {

    CasparCgInstanceData* instance_data = new CasparCgInstanceData;
    env.SetInstanceData(instance_data); // TODO - cleanup


  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, InitServer));
  exports.Set(Napi::String::New(env, "stop"),
              Napi::Function::New(env, StopServer));

  exports.Set(Napi::String::New(env, "parseCommand"),
              Napi::Function::New(env, ParseCommand));

  return exports;
}

NODE_API_MODULE(casparcg, Init)