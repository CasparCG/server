#include "./producer.h"
#include "./promiseWrapper.h"
#include "./stage.h"

#include "../conv.h"
#include "../server.h"

#include <core/producer/cg_proxy.h>
#include <core/video_channel.h>

#include <optional>

struct ParsedChannelLayerArgumentsForCg
{
    const caspar::spl::shared_ptr<std::vector<caspar::spl::shared_ptr<caspar::core::video_channel>>> channels;

    const int channelIndex;
    const int layerIndex;
    const int cgLayer;

    const caspar::spl::shared_ptr<caspar::core::video_channel>& channel;

    Napi::Promise            promise;
    resolveFunc<std::string> resolve;
    rejectFunc               reject;

    ParsedChannelLayerArgumentsForCg(
        const caspar::spl::shared_ptr<std::vector<caspar::spl::shared_ptr<caspar::core::video_channel>>> channels,
        const int                                                                                        channelIndex,
        const int                                                                                        layerIndex,
        const int                                                                                        cgLayer,
        const caspar::spl::shared_ptr<caspar::core::video_channel>&                                      channel,
        Napi::Promise                                                                                    promise,
        resolveFunc<std::string>                                                                         resolve,
        rejectFunc                                                                                       reject)
        : channels(std::move(channels))
        , channelIndex(channelIndex)
        , layerIndex(layerIndex)
        , cgLayer(cgLayer)
        , channel(channel)
        , promise(promise)
        , resolve(resolve)
        , reject(reject)
    {
    }
};

std::optional<ParsedChannelLayerArgumentsForCg>
parseChannelLayerArgumentsForCg(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data, int expectedCount)
{
    Napi::Env env = info.Env();

    if (info.Length() < expectedCount + 4 || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return {};
    }

    auto channelIndex = info[1].As<Napi::Number>().Int32Value();
    auto layerIndex   = info[2].As<Napi::Number>().Int32Value();
    auto cgLayer      = info[3].As<Napi::Number>().Int32Value();

    auto channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return {};
    }

    auto& channel = channels->at(channelIndex - 1);

    auto prom = promiseFuncWrapper<std::string>(env);

    return ParsedChannelLayerArgumentsForCg(
        channels, channelIndex, layerIndex, cgLayer, channel, std::get<0>(prom), std::get<1>(prom), std::get<2>(prom));
}

Napi::Value CgPlay(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArgumentsForCg(info, instance_data, 1);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel->tmp_executor_->begin_invoke([instance_data, parsedArgs] {
        try {
            auto proxy = instance_data->caspar_server->get_cg_proxy(parsedArgs->channelIndex, parsedArgs->layerIndex);

            // if (proxy == caspar::core::cg_proxy::empty()) {
            //     parsedArgs->reject("No CG proxy running on layer");
            //     return;
            // }

            proxy->play(parsedArgs->cgLayer);

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value CgStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArgumentsForCg(info, instance_data, 1);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel->tmp_executor_->begin_invoke([instance_data, parsedArgs] {
        try {
            auto proxy = instance_data->caspar_server->get_cg_proxy(parsedArgs->channelIndex, parsedArgs->layerIndex);

            if (proxy == caspar::core::cg_proxy::empty()) {
                parsedArgs->reject("No CG proxy running on layer");
                return;
            }

            proxy->stop(parsedArgs->cgLayer);

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value CgNext(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArgumentsForCg(info, instance_data, 1);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel->tmp_executor_->begin_invoke([instance_data, parsedArgs] {
        try {
            auto proxy = instance_data->caspar_server->get_cg_proxy(parsedArgs->channelIndex, parsedArgs->layerIndex);

            if (proxy == caspar::core::cg_proxy::empty()) {
                parsedArgs->reject("No CG proxy running on layer");
                return;
            }

            proxy->next(parsedArgs->cgLayer);

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}

Napi::Value CgRemove(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data)
{
    Napi::Env env = info.Env();

    auto parsedArgs = parseChannelLayerArgumentsForCg(info, instance_data, 1);
    if (!parsedArgs)
        return env.Null();

    parsedArgs->channel->tmp_executor_->begin_invoke([instance_data, parsedArgs] {
        try {
            auto proxy = instance_data->caspar_server->get_cg_proxy(parsedArgs->channelIndex, parsedArgs->layerIndex);

            if (proxy == caspar::core::cg_proxy::empty()) {
                parsedArgs->reject("No CG proxy running on layer");
                return;
            }

            proxy->remove(parsedArgs->cgLayer);

            parsedArgs->resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            parsedArgs->reject("Internal error");
        }
    });

    return parsedArgs->promise;
}
