#include "./producer.h"
#include "../conv.h"
#include "../server.h"
#include "./base.h"

#include <common/utf.h>

#include <boost/algorithm/string.hpp>

#include <core/producer/frame_producer.h>
#include <core/producer/transition/sting_producer.h>
#include <core/producer/transition/transition_producer.h>

NodeUnusedProducer::NodeUnusedProducer(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NodeUnusedProducer>(info)
    , _producer(caspar::core::frame_producer::empty())
{
    Napi::Env env = info.Env();

    // Nothing to do
}
Napi::FunctionReference* NodeUnusedProducer::Init(Napi::Env env, Napi::Object exports)
{
    // This method is used to hook the accessor and method callbacks
    Napi::Function func =
        DefineClass(env,
                    "NodeUnusedProducer",
                    {
                        InstanceMethod<&NodeUnusedProducer::IsValid>(
                            "IsValid", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();

    // Create a persistent reference to the class constructor. This will allow
    // a function called on a class prototype and a function
    // called on instance of a class to be distinguished from each other.
    *constructor = Napi::Persistent(func);
    exports.Set("NodeUnusedProducer", func);

    return constructor;
}
caspar::spl::shared_ptr<caspar::core::frame_producer> NodeUnusedProducer::TakeProducer(const Napi::Object& object)
{
    auto my_producer = NodeUnusedProducer::Unwrap(object);
    if (!my_producer || my_producer->_producer == caspar::core::frame_producer::empty()) {
        // Napi::Error::New(env, "Missing producer").ThrowAsJavaScriptException();
        return caspar::core::frame_producer::empty();
    }

    auto new_producer      = std::move(my_producer->_producer);
    my_producer->_producer = caspar::core::frame_producer::empty();

    return new_producer;
}
Napi::Value NodeUnusedProducer::IsValid(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    bool isValid = _producer != caspar::core::frame_producer::empty();
    return Napi::Boolean::New(env, isValid);
}

Napi::Value WrapProducer(const Napi::Env&                                      env,
                         CasparCgInstanceData*                                 instance_data,
                         caspar::spl::shared_ptr<caspar::core::frame_producer> new_producer)
{
    if (new_producer == caspar::core::frame_producer::empty()) {
        return env.Null();
    }

    Napi::Object        result      = instance_data->unused_producer->New({});
    NodeUnusedProducer* resultInner = NodeUnusedProducer::Unwrap(result);
    resultInner->_producer          = std::move(new_producer);

    return result;
}

Napi::Value CreateProducer(const Napi::CallbackInfo& info)
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

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsArray()) {
        Napi::Error::New(env, "Incorrect parameters").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index  = info[0].As<Napi::Number>().Int32Value();
    auto layer_index    = info[1].As<Napi::Number>().Int32Value();
    auto raw_parameters = info[2].As<Napi::Array>();

    auto channels = instance_data->caspar_server->get_channels();

    if (channel_index < 1 || channel_index > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return {};
    }

    auto& channel = channels->at(channel_index - 1);

    std::vector<std::wstring> parameters;
    NapiArrayToStringVector(env, raw_parameters, parameters);

    // TODO - in thread?

    try {
        auto new_producer = instance_data->caspar_server->create_producer(channel.raw_channel, layer_index, parameters);

        return WrapProducer(env, instance_data, new_producer);
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return env.Null();
    }
}

Napi::Value CreateStingTransition(const Napi::CallbackInfo& info)
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

    if (info.Length() < 4 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject() || !info[3].IsObject()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index = info[0].As<Napi::Number>().Int32Value();
    auto layer_index   = info[1].As<Napi::Number>().Int32Value();
    auto producer      = info[2].As<Napi::Object>();
    auto sting_props   = info[3].As<Napi::Object>();

    auto channels = instance_data->caspar_server->get_channels();

    if (channel_index < 1 || channel_index > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto& channel = channels->at(channel_index - 1);

    caspar::core::sting_info parsed_props;
    parsed_props.mask_filename    = caspar::u16(sting_props.Get("mask_filename").As<Napi::String>().Utf8Value());
    parsed_props.overlay_filename = caspar::u16(sting_props.Get("overlay_filename").As<Napi::String>().Utf8Value());
    parsed_props.trigger_point    = sting_props.Get("trigger_point").As<Napi::Number>().Int32Value();
    parsed_props.audio_fade_start = sting_props.Get("audio_fade_start").As<Napi::Number>().Int32Value();
    auto audio_fade_duration      = sting_props.Get("audio_fade_duration");
    if (!audio_fade_duration.IsNull()) {
        parsed_props.audio_fade_duration = audio_fade_duration.As<Napi::Number>().Int32Value();
    }

    auto new_producer = NodeUnusedProducer::TakeProducer(producer);
    if (new_producer == caspar::core::frame_producer::empty()) {
        Napi::Error::New(env, "Missing producer").ThrowAsJavaScriptException();
        return env.Null();
    }

    try {
        auto transition = instance_data->caspar_server->create_sting_transition(
            channel.raw_channel, layer_index, new_producer, parsed_props);

        return WrapProducer(env, instance_data, transition);

    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return env.Null();
    }
}

Napi::Value CreateBasicTransition(const Napi::CallbackInfo& info)
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

    if (info.Length() < 4 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject() || !info[3].IsObject()) {
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto channel_index    = info[0].As<Napi::Number>().Int32Value();
    auto layer_index      = info[1].As<Napi::Number>().Int32Value();
    auto producer         = info[2].As<Napi::Object>();
    auto transition_props = info[3].As<Napi::Object>();

    auto channels = instance_data->caspar_server->get_channels();

    if (channel_index < 1 || channel_index > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto& channel = channels->at(channel_index - 1);

    caspar::core::transition_info parsed_props;
    parsed_props.duration = transition_props.Get("duration").As<Napi::Number>().Int32Value();

    auto direction_str = transition_props.Get("direction").As<Napi::String>().Utf8Value();
    if (!direction_str.empty()) {
        boost::to_upper(direction_str);
        auto direction = caspar::core::parse_transition_direction(direction_str);
        if (!direction) {
            Napi::Error::New(env, "Invalid transition direction").ThrowAsJavaScriptException();
            return env.Null();
        }
        parsed_props.direction = *direction;
    }

    auto type_str = transition_props.Get("type").As<Napi::String>().Utf8Value();
    if (!type_str.empty()) {
        boost::to_upper(type_str);
        auto type = caspar::core::parse_transition_type(type_str);
        if (!type) {
            Napi::Error::New(env, "Invalid transition type").ThrowAsJavaScriptException();
            return env.Null();
        }
        parsed_props.type = *type;
    }

    auto tweener_str = transition_props.Get("tweener").As<Napi::String>().Utf8Value();
    if (!tweener_str.empty()) {
        try {
            parsed_props.tweener = caspar::tweener(caspar::u16(tweener_str));
        } catch (...) {
            Napi::Error::New(env, "Invalid tween type").ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    auto new_producer = NodeUnusedProducer::TakeProducer(producer);
    if (new_producer == caspar::core::frame_producer::empty()) {
        Napi::Error::New(env, "Missing producer").ThrowAsJavaScriptException();
        return env.Null();
    }

    try {
        auto transition = instance_data->caspar_server->create_basic_transition(
            channel.raw_channel, layer_index, new_producer, parsed_props);

        return WrapProducer(env, instance_data, transition);

    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return env.Null();
    }
}