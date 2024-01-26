#include "./stage.h"
#include "./producer.h"
#include "./promiseWrapper.h"

#include "../conv.h"
#include "../server.h"

#include <core/frame/frame_transform.h>
#include <core/producer/frame_producer.h>

#include <optional>

struct ParsedChannelLayerArguments
{
    const caspar::spl::shared_ptr<std::vector<caspar::protocol::amcp::channel_context>> channels;

    const int channelIndex;
    const int layerIndex;

    const caspar::protocol::amcp::channel_context& channel;

    Napi::Promise            promise;
    resolveFunc<std::string> resolve;
    rejectFunc               reject;

    ParsedChannelLayerArguments(
        const caspar::spl::shared_ptr<std::vector<caspar::protocol::amcp::channel_context>> channels,
        const int                                                                           channelIndex,
        const int                                                                           layerIndex,
        const caspar::protocol::amcp::channel_context&                                      channel,
        Napi::Promise                                                                       promise,
        resolveFunc<std::string>                                                            resolve,
        rejectFunc                                                                          reject)
        : channels(std::move(channels))
        , channelIndex(channelIndex)
        , layerIndex(layerIndex)
        , channel(channel)
        , promise(promise)
        , resolve(resolve)
        , reject(reject)
    {
    }
};

std::optional<ParsedChannelLayerArguments>
parseChannelLayerArguments(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data, int expectedCount)
{
    Napi::Env env = info.Env();

    if (info.Length() < expectedCount + 3 || !info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return {};
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[2].As<Napi::Number>().Int32Value();

    auto channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return {};
    }

    auto& channel = channels->at(channelIndex - 1);

    auto prom = promiseFuncWrapper<std::string>(env);

    return ParsedChannelLayerArguments(
        channels, channelIndex, layerIndex, channel, std::get<0>(prom), std::get<1>(prom), std::get<2>(prom));
}

Napi::Value StagePlay(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 0);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs] {
        try {
            parsedArgs->channel.stage->play(parsedArgs->layerIndex).get();

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}
Napi::Value StagePreview(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 0);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs] {
        try {
            parsedArgs->channel.stage->preview(parsedArgs->layerIndex).get();

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value StageLoad(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 3);
    if (!parsedArgs)
        return env.Null();

    if (!info[3].IsObject() || !info[4].IsBoolean() || !info[5].IsBoolean()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto producer_obj = info[3].As<Napi::Object>();
    auto preview      = info[4].As<Napi::Boolean>().Value();
    auto autoPlay     = info[5].As<Napi::Boolean>().Value();

    auto producer = NodeUnusedProducer::TakeProducer(producer_obj);
    if (producer == caspar::core::frame_producer::empty()) {
        Napi::Error::New(env, "Missing producer").ThrowAsJavaScriptException();
        return env.Null();
    }

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs, producer, preview, autoPlay] {
        try {
            parsedArgs->channel.stage->load(parsedArgs->layerIndex, producer, preview, autoPlay);

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value StagePause(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 0);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs] {
        try {
            parsedArgs->channel.stage->pause(parsedArgs->layerIndex).get();

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value StageResume(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 0);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs] {
        try {
            parsedArgs->channel.stage->resume(parsedArgs->layerIndex).get();

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value StageStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 0);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs] {
        try {
            parsedArgs->channel.stage->stop(parsedArgs->layerIndex).get();

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
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

    auto prom    = promiseFuncWrapper<std::string>(env);
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

    auto parsedArgs = parseChannelLayerArguments(info, instance_data, 1);
    if (!parsedArgs)
        return env.Null();

    if (!info[3].IsArray()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto raw_parameters = info[3].As<Napi::Array>();

    if (raw_parameters.Length() == 0) {
        Napi::Error::New(env, "No parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::vector<std::wstring> parameters;
    NapiArrayToStringVector(env, raw_parameters, parameters);

    parsedArgs->channel.tmp_executor_->begin_invoke([parsedArgs, parameters] {
        try {
            const auto result = parsedArgs->channel.stage->call(parsedArgs->layerIndex, parameters).get();

            parsedArgs->resolve(caspar::u8(result));
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
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

    auto prom    = promiseFuncWrapper<std::string>(env);
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

    auto prom    = promiseFuncWrapper<std::string>(env);
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

template <typename T>
Napi::Array ConvertNumberArray(const Napi::Env& env, const std::array<T, 2>& source)
{
    Napi::Array value = Napi::Array::New(env, 2);
    value.Set(uint32_t(0), Napi::Number::New(env, source[0]));
    value.Set(uint32_t(1), Napi::Number::New(env, source[1]));
    return value;
}

Napi::Value GetLayerMixerProperties(const Napi::CallbackInfo& info)
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
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return {};
    }

    auto channelIndex = info[0].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[1].As<Napi::Number>().Int32Value();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch = channels->at(channelIndex - 1);

    auto convert = [](const Napi::Env& env, const caspar::core::frame_transform transform) {
        Napi::Object image = Napi::Object::New(env);
        image.Set("opacity", Napi::Number::New(env, transform.image_transform.opacity));
        image.Set("contrast", Napi::Number::New(env, transform.image_transform.contrast));
        image.Set("brightness", Napi::Number::New(env, transform.image_transform.brightness));
        image.Set("saturation", Napi::Number::New(env, transform.image_transform.saturation));

        image.Set("anchor", ConvertNumberArray(env, transform.image_transform.anchor));
        image.Set("fill_translation", ConvertNumberArray(env, transform.image_transform.fill_translation));
        image.Set("fill_scale", ConvertNumberArray(env, transform.image_transform.fill_scale));
        image.Set("clip_translation", ConvertNumberArray(env, transform.image_transform.clip_translation));
        image.Set("clip_scale", ConvertNumberArray(env, transform.image_transform.clip_scale));
        image.Set("angle", Napi::Number::New(env, transform.image_transform.angle));

        Napi::Object crop = Napi::Object::New(env);
        crop.Set("ul", ConvertNumberArray(env, transform.image_transform.crop.ul));
        crop.Set("lr", ConvertNumberArray(env, transform.image_transform.crop.lr));
        image.Set("crop", crop);

        Napi::Object perspective = Napi::Object::New(env);
        perspective.Set("ul", ConvertNumberArray(env, transform.image_transform.perspective.ul));
        perspective.Set("ur", ConvertNumberArray(env, transform.image_transform.perspective.ur));
        perspective.Set("lr", ConvertNumberArray(env, transform.image_transform.perspective.lr));
        perspective.Set("ll", ConvertNumberArray(env, transform.image_transform.perspective.ll));
        image.Set("perspective", perspective);

        Napi::Object levels = Napi::Object::New(env);
        levels.Set("min_input", Napi::Number::New(env, transform.image_transform.levels.min_input));
        levels.Set("max_input", Napi::Number::New(env, transform.image_transform.levels.max_input));
        levels.Set("gamma", Napi::Number::New(env, transform.image_transform.levels.gamma));
        levels.Set("min_output", Napi::Number::New(env, transform.image_transform.levels.min_output));
        levels.Set("max_output", Napi::Number::New(env, transform.image_transform.levels.max_output));
        image.Set("levels", levels);

        Napi::Object chroma = Napi::Object::New(env);
        chroma.Set("enable", Napi::Boolean::New(env, transform.image_transform.chroma.enable));
        chroma.Set("show_mask", Napi::Boolean::New(env, transform.image_transform.chroma.show_mask));
        chroma.Set("target_hue", Napi::Number::New(env, transform.image_transform.chroma.target_hue));
        chroma.Set("hue_width", Napi::Number::New(env, transform.image_transform.chroma.hue_width));
        chroma.Set("min_saturation", Napi::Number::New(env, transform.image_transform.chroma.min_saturation));
        chroma.Set("min_brightness", Napi::Number::New(env, transform.image_transform.chroma.min_brightness));
        chroma.Set("softness", Napi::Number::New(env, transform.image_transform.chroma.softness));
        chroma.Set("spill_suppress", Napi::Number::New(env, transform.image_transform.chroma.spill_suppress));
        chroma.Set("spill_suppress_saturation",
                   Napi::Number::New(env, transform.image_transform.chroma.spill_suppress_saturation));
        image.Set("chroma", chroma);

        image.Set("is_key", Napi::Boolean::New(env, transform.image_transform.is_key));
        image.Set("invert", Napi::Boolean::New(env, transform.image_transform.invert));
        image.Set("is_mix", Napi::Boolean::New(env, transform.image_transform.is_mix));
        image.Set("blend_mode",
                  Napi::String::New(env, caspar::u8(get_blend_mode(transform.image_transform.blend_mode))));
        image.Set("layer_depth", Napi::Number::New(env, transform.image_transform.layer_depth));

        Napi::Object audio = Napi::Object::New(env);
        audio.Set("volume", Napi::Boolean::New(env, transform.audio_transform.volume));

        Napi::Object value = Napi::Object::New(env);
        value.Set("image", image);
        value.Set("audio", audio);

        return value;
    };

    auto prom    = promiseFuncWrapper2<caspar::core::frame_transform, decltype(convert)>(env, convert);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch.tmp_executor_->begin_invoke([=] {
        try {
            auto transform = ch.stage->get_current_transform(layerIndex).get();

            resolve(transform);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}