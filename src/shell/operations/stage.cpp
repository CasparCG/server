#include "./stage.h"
#include "./promiseWrapper.h"

#include "../server.h"

Napi::Value StagePause(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[2].As<Napi::Number>().Int32Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            ch1.stage->pause(layerIndex).get();

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value StageResume(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[2].As<Napi::Number>().Int32Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            ch1.stage->resume(layerIndex).get();

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value StageStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[2].As<Napi::Number>().Int32Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            ch1.stage->stop(layerIndex).get();

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value StageClear(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[1].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info.Length() > 2 && !info[2].IsNull() ? info[2].As<Napi::Number>().Int32Value() : -1;

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            if (layerIndex < 0) {
                ch1.stage->clear().get();
            } else {
                ch1.stage->clear(layerIndex).get();
            }

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

// Napi::Value StageClearAll(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
// {
//     Napi::Env env = info.Env();

//     auto prom    = promiseFuncWrapper(env);
//     auto resolve = std::get<1>(prom);
//     auto reject  = std::get<2>(prom);

//     auto& channels = instance_data->caspar_server->get_channels();

//     // TODO - reimplement this

//     // ch1.tmp_executor_->begin_invoke([channels] {
//     //     try {
//     //         for (auto& ch : *channels) {
//     //             ch.stage->clear().get();
//     //         }

//     //         resolve("");
//     //     } catch (...) {
//     //         CASPAR_LOG_CURRENT_EXCEPTION();
//     //         reject("Internal error");
//     //     }
//     // });

//     return std::get<0>(prom);
// }

Napi::Value StageCall(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 4 || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsArray()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex   = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex     = info[2].As<Napi::Number>().Int32Value();
    auto raw_parameters = info[3].As<Napi::Array>();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (raw_parameters.Length() == 0) {
        Napi::Error::New(env, "No parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::vector<std::wstring> parameters;
    parameters.reserve(raw_parameters.Length());
    for (size_t i = 0; i < raw_parameters.Length(); i++) {
        auto str = static_cast<Napi::Value>(raw_parameters[i]).ToString();
        parameters.emplace_back(caspar::u16(str.Utf8Value()));
    }

    auto ch1 = channels->at(channelIndex - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            const auto result = ch1.stage->call(layerIndex, parameters).get();

            resolve(caspar::u8(result));
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value StageSwapChannel(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex1   = info[1].As<Napi::Number>().Int32Value();
    auto channelIndex2   = info[2].As<Napi::Number>().Int32Value();
    bool swap_transforms = info.Length() > 3 && info[3].As<Napi::Boolean>().Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex1 < 1 || channelIndex1 > channels->size() || channelIndex2 < 1 ||
        channelIndex2 > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex1 - 1);
    auto ch2 = channels->at(channelIndex2 - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            ch1.stage->swap_layers(ch2.stage, swap_transforms).get();

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

Napi::Value StageSwapLayer(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    if (info.Length() < 5 || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channelIndex1   = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex1     = info[2].As<Napi::Number>().Int32Value();
    auto channelIndex2   = info[3].As<Napi::Number>().Int32Value();
    auto layerIndex2     = info[4].As<Napi::Number>().Int32Value();
    bool swap_transforms = info.Length() > 5 && info[5].As<Napi::Boolean>().Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex1 < 1 || channelIndex1 > channels->size() || channelIndex2 < 1 ||
        channelIndex2 > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch1 = channels->at(channelIndex1 - 1);
    auto ch2 = channels->at(channelIndex2 - 1);

    auto prom    = promiseFuncWrapper(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch1.tmp_executor_->begin_invoke([=] {
        try {
            ch1.stage->swap_layer(layerIndex1, layerIndex2, ch2.stage, swap_transforms);

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}