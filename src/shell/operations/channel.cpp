#include "./channel.h"
#include "../conv.h"
#include "../server.h"
#include "./base.h"
#include "./promiseWrapper.h"

#include <common/utf.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <set>

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

Napi::Value SetChannelFormat(const Napi::CallbackInfo& info)
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

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsString()) {
        Napi::Error::New(env, "Incorrect parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index = info[0].As<Napi::Number>().Int32Value();
    auto format_name   = info[1].As<Napi::String>().Utf8Value();

    auto channels = instance_data->caspar_server->get_channels();

    if (channel_index < 1 || channel_index > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto& channel = channels->at(channel_index - 1);

    auto format_desc = instance_data->caspar_server->get_video_format_desc(caspar::u16(format_name));
    if (format_desc.format == caspar::core::video_format::invalid) {
        Napi::Error::New(env, "Invalid video mode").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto prom    = promiseFuncWrapper<std::string>(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    // TODO - is this the correct thread?
    channel.tmp_executor_->begin_invoke([instance_data, channel, format_desc, resolve, reject] {
        try {
            channel.raw_channel->stage()->video_format_desc(format_desc);

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}
