#include "./consumer.h"
#include "../conv.h"
#include "../server.h"
#include "./base.h"
#include "./promiseWrapper.h"

#include <common/utf.h>

#include <boost/algorithm/string.hpp>

#include <core/consumer/frame_consumer.h>
#include <core/video_channel.h>

Napi::Value AddConsumer(const Napi::CallbackInfo& info)
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

    if (info.Length() < 3 || !info[0].IsNumber() || (!info[1].IsNumber() && !info[1].IsNull()) || !info[2].IsArray()) {
        Napi::Error::New(env, "Incorrect parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index  = info[0].As<Napi::Number>().Int32Value();
    auto layer_index    = (info[1].IsNull()) ? -1 : info[1].As<Napi::Number>().Int32Value();
    auto raw_parameters = info[2].As<Napi::Array>();

    auto channels = instance_data->caspar_server->get_channels();

    if (channel_index < 1 || channel_index > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto& channel = channels->at(channel_index - 1);

    std::vector<std::wstring> parameters;
    NapiArrayToStringVector(env, raw_parameters, parameters);

    auto prom    = promiseFuncWrapper<int>(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    // TODO - is this the correct thread?
    channel->tmp_executor_->begin_invoke([instance_data, channel, layer_index, parameters, resolve, reject] {
        try {
            auto index = instance_data->caspar_server->add_consumer_from_tokens(channel, layer_index, parameters);

            resolve(index);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value RemoveConsumerByPort(const Napi::CallbackInfo& info)
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

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::Error::New(env, "Incorrect parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index = info[0].As<Napi::Number>().Int32Value();
    auto layer_index   = info[1].As<Napi::Number>().Int32Value();

    // TODO - thread?
    try {
        auto success = instance_data->caspar_server->remove_consumer_by_port(channel_index, layer_index);

        return Napi::Boolean::New(env, success);
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value RemoveConsumerByParams(const Napi::CallbackInfo& info)
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

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsArray()) {
        Napi::Error::New(env, "Incorrect parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index  = info[0].As<Napi::Number>().Int32Value();
    auto raw_parameters = info[1].As<Napi::Array>();

    if (raw_parameters.Length() == 0) {
        return Napi::Boolean::New(env, false);
    }

    std::vector<std::wstring> parameters;
    NapiArrayToStringVector(env, raw_parameters, parameters);

    // TODO - thread?
    try {
        auto success = instance_data->caspar_server->remove_consumer_by_params(channel_index, parameters);

        return Napi::Boolean::New(env, success);
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return Napi::Boolean::New(env, false);
    }
}
