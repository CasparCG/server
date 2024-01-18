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

#include <protocol/amcp/AMCPCommandQueue.h>

#include <common/env.h>
#include <common/except.h>
#include <common/log.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/stacktrace.hpp>

#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>

#include <atomic>
#include <future>
#include <thread>

#include <clocale>
#include <csignal>

#include "napi.h"

#include "./conv.h"
#include "./operations/base.h"
#include "./operations/producer.h"
#include "./operations/stage.h"
#include "./operations/util.h"

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

class NodeAMCPCommand : public Napi::ObjectWrap<NodeAMCPCommand>
{
  public:
    static Napi::FunctionReference* Init(Napi::Env env, Napi::Object exports)
    {
        // This method is used to hook the accessor and method callbacks
        Napi::Function func =
            DefineClass(env,
                        "AMCPCommand",
                        {
                            // StaticMethod<&NodeAMCPCommand::CreateNewItem>(
                            //     "Create", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();

        // Create a persistent reference to the class constructor. This will allow
        // a function called on a class prototype and a function
        // called on instance of a class to be distinguished from each other.
        *constructor = Napi::Persistent(func);
        exports.Set("AMCPCommand", func);

        return constructor;
    }

    NodeAMCPCommand(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<NodeAMCPCommand>(info)
    {
        Napi::Env env = info.Env();

        CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
        if (!instance_data) {
            Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
                .ThrowAsJavaScriptException();
            return;
        }

        if (!instance_data->caspar_server) {
            Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
            return;
        }

        if (info.Length() < 4) {
            Napi::Error::New(env, "Not enough parameters").ThrowAsJavaScriptException();
            return;
        }

        auto command_name  = info[0].As<Napi::String>().Utf8Value();
        int  channel_index = info[1].As<Napi::Number>().Int32Value();
        int  layer_index   = info[2].As<Napi::Number>().Int32Value();
        auto raw_tokens    = info[3].As<Napi::Array>();

        std::list<std::wstring> tokens;
        // tokens.resize(raw_tokens.Length());

        for (size_t i = 0; i < raw_tokens.Length(); i++) {
            auto str = static_cast<Napi::Value>(raw_tokens[i]).ToString();
            tokens.emplace_back(caspar::u16(str.Utf8Value()));
        }

        // Create a dummy client which prints amcp responses to console.
        auto console_client = L"console";

        auto cmd = instance_data->caspar_server->get_amcp_command_repository()->parse_command(
            console_client, caspar::u16(command_name), channel_index, layer_index, tokens);

        if (!cmd) {
            Napi::Error::New(env, "Failed to parse command2").ThrowAsJavaScriptException();
            return;
        }

        this->_cmd = std::move(cmd);
    }

    // static Napi::Value CreateNewItem(const Napi::CallbackInfo& info)
    // {
    //     Napi::Env env = info.Env();

    //     CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    //     if (!instance_data) {
    //         Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
    //             .ThrowAsJavaScriptException();
    //         return env.Null();
    //     }

    //     return instance_data->amcp_command->New({Napi::Number::New(info.Env(), 42)});
    // }

    bool                                                 _executed = false;
    std::shared_ptr<caspar::protocol::amcp::AMCPCommand> _cmd;
};

Napi::Value InitServer(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (instance_data->caspar_server) {
        Napi::Error::New(env, "Server is already running, it must be stopped before you can call init again")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO - ensure not in a worker, or that multiple servers arent run in the same process

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::Error::New(env, "Expected configuration object").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto configuration = info[0].As<Napi::Object>();

    using namespace caspar;

    // setup_global_locale();

    setup_prerequisites();

    // Increase process priority.
    increase_process_priority();

    // TODO: remove the use of this config file. everything needs to be passed from node
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

        print_info();

        // Create server object which initializes channels, protocols and controllers.
        instance_data->caspar_server.reset(new server());

        // For example CEF resets the global locale, so this is to reset it back to "our" preference.
        setup_global_locale();

        // std::wstringstream                                      str;
        // boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
        // boost::property_tree::write_xml(str, env::properties(), w);
        // CASPAR_LOG(info) << boost::filesystem::absolute(config_file_name).lexically_normal()
        //                  << L":\n-----------------------------------------\n"
        //                  << str.str() << L"-----------------------------------------";

        instance_data->caspar_server->start();

        instance_data->amcp_queue = std::make_shared<protocol::amcp::AMCPCommandQueue>(
            L"NodeJS Test", instance_data->caspar_server->get_amcp_command_repository()->channels());

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

Napi::Value StopServer(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        // Nothing to do
        return env.Null();
    }

    instance_data->amcp_queue.reset();

    instance_data->caspar_server = nullptr;

    return env.Null();
}

Napi::Value ExecuteCommandBatch(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server || !instance_data->amcp_queue) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 1 || !info[0].IsArray()) {
        Napi::Error::New(env, "Expected array of commands").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto raw_commands = info[0].As<Napi::Array>();

    if (raw_commands.Length() == 0) {
        // TODO - is this ok?
        return env.Null();
    }

    std::vector<std::shared_ptr<caspar::protocol::amcp::AMCPCommand>> commands;
    commands.reserve(raw_commands.Length());

    for (size_t i = 0; i < raw_commands.Length(); i++) {
        auto raw_cmd = static_cast<Napi::Value>(raw_commands[i]).As<Napi::Object>();
        if (raw_cmd.InstanceOf(instance_data->amcp_command->Value())) {
            auto cmd_wrapper = NodeAMCPCommand::Unwrap(raw_cmd);

            if (cmd_wrapper->_executed) {
                // TODO - this guard
            }

            commands.emplace_back(cmd_wrapper->_cmd);
        }
    }

    // TODO - the commands get mutated. they should be blocked from being executed multiple times

    // TODO - avoid this blocking...
    std::wstring error_response = instance_data->amcp_queue->QueueCommandBatch(commands).get();

    size_t count = commands.size();
    if (!error_response.empty()) {
        count++;
    }

    auto response = Napi::Array::New(env, count);
    for (size_t i = 0; i < commands.size(); i++) {
        response.Set(i, caspar::u8(commands[i]->result()));
    }
    if (!error_response.empty()) {
        response.Set(count - 1, caspar::u8(error_response));
    }

    // TODO - make it so that the call above doesn't return a value, but dispatches some work which will post back
    // to nodejs and resolve the promise

    // return Napi::String::New(env, caspar::u8(error_response));
    return response;
}

Napi::Value ConfigAddOscPredefinedClient(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::Error::New(env, "Expected address and port").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto address = info[0].As<Napi::String>();
    auto port    = info[1].As<Napi::Number>();

    bool success = instance_data->caspar_server->add_osc_predefined_client(address.Utf8Value(), port.Int32Value());

    return Napi::Boolean::New(env, success);
}

Napi::Value AddChannelConsumer(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsObject()) {
        Napi::Error::New(env, "Expected channel and config").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index = info[0].As<Napi::Number>();
    auto config        = info[1].As<Napi::Object>();

    boost::property_tree::wptree boost_config;
    if (!NapiObjectToBoostPropertyTree(env, config, boost_config)) {
        return env.Null();
    }

    int port = instance_data->caspar_server->add_consumer_from_xml(channel_index.Int32Value() - 1, boost_config);
    if (port == -1) {
        Napi::Error::New(env, "Invalid consumer config").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Number::New(env, port);
}

Napi::Value AddChannel(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::Error::New(env, "Expected video mode").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto video_mode = info[0].As<Napi::String>();

    // TODO - calling this once amcp has begun is not safe

    int channel_index = instance_data->caspar_server->add_channel(caspar::u16(video_mode.Utf8Value()));
    if (channel_index == -1) {
        Napi::Error::New(env, "Failed to add channel").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Number::New(env, channel_index);
}

Napi::Value ConfigAddCustomVideoFormat(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::Error::New(env, "Expected video format definition").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto obj = info[0].As<Napi::Object>();

    auto id          = obj.Get("id").As<Napi::String>().Utf8Value();
    auto width       = obj.Get("width").As<Napi::Number>().Int32Value();
    auto height      = obj.Get("height").As<Napi::Number>().Int32Value();
    auto fieldCount  = obj.Get("fieldCount").As<Napi::Number>().Int32Value();
    auto timeScale   = obj.Get("timeScale").As<Napi::Number>().Int32Value();
    auto duration    = obj.Get("duration").As<Napi::Number>().Int32Value();
    auto cadence_str = obj.Get("cadence").As<Napi::String>().Utf8Value();

    std::set<std::wstring> cadence_parts;
    boost::split(cadence_parts, cadence_str, boost::is_any_of(L", "));

    std::vector<int> cadence;
    for (auto& cad : cadence_parts) {
        if (cad == L"")
            continue;

        const int c = std::stoi(cad);
        cadence.push_back(c);
    }

    if (cadence.size() == 0) {
        const int c = static_cast<int>(48000 / (static_cast<double>(timeScale) / duration) + 0.5);
        cadence.push_back(c);
    }

    const auto new_format = caspar::core::video_format_desc(caspar::core::video_format::custom,
                                                            fieldCount,
                                                            width,
                                                            height,
                                                            width,
                                                            height,
                                                            timeScale,
                                                            duration,
                                                            caspar::u16(id),
                                                            cadence);

    instance_data->caspar_server->add_video_format_desc(caspar::u16(id), new_format);

    // TODO - calling this once amcp has begun may not be safe

    return env.Null();
}

Napi::Value OpenDiag(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    caspar::core::diagnostics::osd::show_graphs(true);

    return env.Null();
}

Napi::Value CallStageMethod(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    CasparCgInstanceData* instance_data = env.GetInstanceData<CasparCgInstanceData>();
    if (!instance_data) {
        Napi::Error::New(env, "Module is not initialised correctly. This is likely a bug!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!instance_data->caspar_server) {
        Napi::Error::New(env, "Server is not running").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::Error::New(env, "Expected command").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto command = info[0].As<Napi::String>().Utf8Value();

    if (command == "load") {
        return StageLoad(info, instance_data);
    } else if (command == "pause") {
        return StagePause(info, instance_data);
    } else if (command == "resume") {
        return StageResume(info, instance_data);
    } else if (command == "stop") {
        return StageStop(info, instance_data);
    } else if (command == "clear") {
        return StageClear(info, instance_data);
    } else if (command == "call") {
        return StageCall(info, instance_data);
    } else if (command == "swapChannel") {
        return StageSwapChannel(info, instance_data);
    } else if (command == "swapLayer") {
        return StageSwapLayer(info, instance_data);
    } else {
        Napi::Error::New(env, "Unknown command").ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    CasparCgInstanceData* instance_data = new CasparCgInstanceData;
    env.SetInstanceData(instance_data); // TODO - cleanup

    instance_data->amcp_command    = NodeAMCPCommand::Init(env, exports);
    instance_data->unused_producer = NodeUnusedProducer::Init(env, exports);

    exports.Set(Napi::String::New(env, "version"), Napi::String::New(env, caspar::u8(caspar::env::version())));

    exports.Set(Napi::String::New(env, "init"), Napi::Function::New(env, InitServer));
    exports.Set(Napi::String::New(env, "stop"), Napi::Function::New(env, StopServer));

    exports.Set(Napi::String::New(env, "ConfigAddOscPredefinedClient"),
                Napi::Function::New(env, ConfigAddOscPredefinedClient));
    exports.Set(Napi::String::New(env, "ConfigAddCustomVideoFormat"),
                Napi::Function::New(env, ConfigAddCustomVideoFormat));

    exports.Set(Napi::String::New(env, "CreateProducer"), Napi::Function::New(env, CreateProducer));
    exports.Set(Napi::String::New(env, "CreateStingTransition"), Napi::Function::New(env, CreateStingTransition));
    exports.Set(Napi::String::New(env, "CreateBasicTransition"), Napi::Function::New(env, CreateBasicTransition));

    exports.Set(Napi::String::New(env, "AddChannelConsumer"), Napi::Function::New(env, AddChannelConsumer));
    exports.Set(Napi::String::New(env, "AddChannel"), Napi::Function::New(env, AddChannel));
    exports.Set(Napi::String::New(env, "CallStageMethod"), Napi::Function::New(env, CallStageMethod));

    exports.Set(Napi::String::New(env, "OpenDiag"), Napi::Function::New(env, OpenDiag));
    exports.Set(Napi::String::New(env, "FindCaseInsensitive"), Napi::Function::New(env, FindCaseInsensitive));

    exports.Set(Napi::String::New(env, "executeCommandBatch"), Napi::Function::New(env, ExecuteCommandBatch));

    return exports;
}

NODE_API_MODULE(casparcg, Init)