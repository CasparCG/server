#include "./producer.h"
#include "./promiseWrapper.h"
#include "./stage.h"

#include "../conv.h"
#include "../server.h"

#include <core/frame/frame_transform.h>
#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>
#include <core/video_channel.h>

#include <optional>

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

    ch->tmp_executor_->begin_invoke([=] {
        try {
            auto transform = ch->stage().get_current_transform(layerIndex).get();

            resolve(transform);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}

std::optional<double> ParseOptionalDouble(const Napi::Object& fragment, const std::string& key)
{
    auto value = fragment.Get(key);
    if (value.IsNumber()) {
        return value.As<Napi::Number>().DoubleValue();
    } else {
        return {};
    }
}

std::optional<bool> ParseOptionalBoolean(const Napi::Object& fragment, const std::string& key)
{
    auto value = fragment.Get(key);
    if (value.IsBoolean()) {
        return value.As<Napi::Boolean>().Value();
    } else {
        return {};
    }
}

std::optional<int32_t> ParseOptionalInt32(const Napi::Object& fragment, const std::string& key)
{
    auto value = fragment.Get(key);
    if (value.IsNumber()) {
        return value.As<Napi::Number>().Int32Value();
    } else {
        return {};
    }
}

std::array<double, 2> ParseDoubleArray(const Napi::Value& value)
{
    auto array = value.As<Napi::Array>();

    std::array<double, 2> result = {array.Get((uint32_t)0).As<Napi::Number>().DoubleValue(),
                                    array.Get((uint32_t)1).As<Napi::Number>().DoubleValue()};
    return result;
}

std::optional<std::array<double, 2>> ParseOptionalDoubleArray(const Napi::Object& fragment, const std::string& key)
{
    auto value = fragment.Get(key);
    if (value.IsArray()) {
        return ParseDoubleArray(value);
    } else {
        return {};
    }
}

std::optional<std::string> ParseOptionalString(const Napi::Object& fragment, const std::string& key)
{
    auto value = fragment.Get(key);
    if (value.IsString()) {
        return value.As<Napi::String>().Utf8Value();
    } else {
        return {};
    }
}

struct partial_image_transform final
{
    partial_image_transform() = default;

    std::optional<double> opacity;
    std::optional<double> contrast;
    std::optional<double> brightness;
    std::optional<double> saturation;

    std::optional<std::array<double, 2>>   anchor;
    std::optional<std::array<double, 2>>   fill_translation;
    std::optional<std::array<double, 2>>   fill_scale;
    std::optional<std::array<double, 2>>   clip_translation;
    std::optional<std::array<double, 2>>   clip_scale;
    std::optional<double>                  angle;
    std::optional<caspar::core::rectangle> crop;
    std::optional<caspar::core::corners>   perspective;
    std::optional<caspar::core::levels>    levels;
    std::optional<caspar::core::chroma>    chroma;

    std::optional<bool>                     is_key;
    std::optional<bool>                     invert;
    std::optional<bool>                     is_mix;
    std::optional<caspar::core::blend_mode> blend_mode;
    std::optional<int>                      layer_depth;

    std::optional<double> audio_volume;
};

caspar::core::stage::transform_func_t createMixerTransform(const Napi::Env& env, const Napi::Object& fragment)
{
    auto imageFragment = fragment.Get("image");
    auto audioFragment = fragment.Get("audio");

    partial_image_transform partial;
    if (imageFragment.IsObject()) {
        auto imageFragment2 = imageFragment.As<Napi::Object>();

        partial.opacity    = ParseOptionalDouble(imageFragment2, "opacity");
        partial.contrast   = ParseOptionalDouble(imageFragment2, "contrast");
        partial.brightness = ParseOptionalDouble(imageFragment2, "brightness");
        partial.saturation = ParseOptionalDouble(imageFragment2, "saturation");

        partial.anchor           = ParseOptionalDoubleArray(imageFragment2, "anchor");
        partial.fill_translation = ParseOptionalDoubleArray(imageFragment2, "fill_translation");
        partial.fill_scale       = ParseOptionalDoubleArray(imageFragment2, "fill_scale");
        partial.clip_translation = ParseOptionalDoubleArray(imageFragment2, "clip_translation");
        partial.clip_scale       = ParseOptionalDoubleArray(imageFragment2, "clip_scale");
        partial.angle            = ParseOptionalDouble(imageFragment2, "angle");

        auto raw_crop = fragment.Get("crop");
        if (raw_crop.IsObject()) {
            auto raw_crop_obj = raw_crop.As<Napi::Object>();
            partial.crop      = {ParseDoubleArray(raw_crop_obj.Get("ul")), ParseDoubleArray(raw_crop_obj.Get("lr"))};
        }
        auto raw_perspective = fragment.Get("perspective");
        if (raw_perspective.IsObject()) {
            auto raw_perspective_obj = raw_perspective.As<Napi::Object>();
            partial.perspective      = {ParseDoubleArray(raw_perspective_obj.Get("ul")),
                                        ParseDoubleArray(raw_perspective_obj.Get("ur")),
                                        ParseDoubleArray(raw_perspective_obj.Get("lr")),
                                        ParseDoubleArray(raw_perspective_obj.Get("ll"))};
        }
        auto raw_levels = fragment.Get("levels");
        if (raw_levels.IsObject()) {
            auto raw_levels_obj = raw_levels.As<Napi::Object>();
            partial.levels      = {raw_levels_obj.Get("min_input").As<Napi::Number>().DoubleValue(),
                                   raw_levels_obj.Get("max_input").As<Napi::Number>().DoubleValue(),
                                   raw_levels_obj.Get("gamma").As<Napi::Number>().DoubleValue(),
                                   raw_levels_obj.Get("min_output").As<Napi::Number>().DoubleValue(),
                                   raw_levels_obj.Get("max_output").As<Napi::Number>().DoubleValue()};
        }
        auto raw_chroma = fragment.Get("chroma");
        if (raw_chroma.IsObject()) {
            auto raw_chroma_obj = raw_chroma.As<Napi::Object>();
            partial.chroma      = {raw_chroma_obj.Get("enable").As<Napi::Boolean>().Value(),
                                   raw_chroma_obj.Get("show_mask").As<Napi::Boolean>().Value(),
                                   raw_chroma_obj.Get("target_hue").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("hue_width").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("min_saturation").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("min_brightness").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("softness").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("spill_suppress").As<Napi::Number>().DoubleValue(),
                                   raw_chroma_obj.Get("spill_suppress_saturation").As<Napi::Number>().DoubleValue()};
        }

        partial.is_key  = ParseOptionalBoolean(imageFragment2, "is_key");
        partial.invert  = ParseOptionalBoolean(imageFragment2, "invert");
        partial.is_mix  = ParseOptionalBoolean(imageFragment2, "is_mix");
        auto blend_mode = ParseOptionalString(imageFragment2, "blend_mode");
        if (blend_mode.has_value())
            partial.blend_mode = caspar::core::get_blend_mode(caspar::u16(*blend_mode));
        partial.layer_depth = ParseOptionalInt32(imageFragment2, "layer_depth");
    }
    if (audioFragment.IsObject()) {
        auto audioFragment2 = audioFragment.As<Napi::Object>();

        partial.audio_volume = ParseOptionalDouble(audioFragment2, "volume");
    }

    return [partial](caspar::core::frame_transform transform) -> caspar::core::frame_transform {
        // Image
        if (partial.opacity.has_value())
            transform.image_transform.opacity = *partial.opacity;
        if (partial.contrast.has_value())
            transform.image_transform.contrast = *partial.contrast;
        if (partial.brightness.has_value())
            transform.image_transform.brightness = *partial.brightness;
        if (partial.saturation.has_value())
            transform.image_transform.saturation = *partial.saturation;

        if (partial.anchor.has_value())
            transform.image_transform.anchor = *partial.anchor;
        if (partial.fill_translation.has_value())
            transform.image_transform.fill_translation = *partial.fill_translation;
        if (partial.fill_scale.has_value())
            transform.image_transform.fill_scale = *partial.fill_scale;
        if (partial.clip_translation.has_value())
            transform.image_transform.clip_translation = *partial.clip_translation;
        if (partial.clip_scale.has_value())
            transform.image_transform.clip_scale = *partial.clip_scale;
        if (partial.angle.has_value())
            transform.image_transform.angle = *partial.angle;
        if (partial.crop.has_value())
            transform.image_transform.crop = *partial.crop;
        if (partial.perspective.has_value())
            transform.image_transform.perspective = *partial.perspective;
        if (partial.levels.has_value())
            transform.image_transform.levels = *partial.levels;
        if (partial.chroma.has_value())
            transform.image_transform.chroma = *partial.chroma;

        if (partial.is_key.has_value())
            transform.image_transform.is_key = *partial.is_key;
        if (partial.invert.has_value())
            transform.image_transform.invert = *partial.invert;
        if (partial.is_mix.has_value())
            transform.image_transform.is_mix = *partial.is_mix;
        if (partial.blend_mode.has_value())
            transform.image_transform.blend_mode = *partial.blend_mode;
        if (partial.layer_depth.has_value())
            transform.image_transform.layer_depth = *partial.layer_depth;

        // Audio
        if (partial.audio_volume.has_value())
            transform.audio_transform.volume = *partial.audio_volume;

        return transform;
    };
}

Napi::Value ApplyTransforms(const Napi::CallbackInfo& info)
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
        Napi::Error::New(env, "Not enough arguments").ThrowAsJavaScriptException();
        return {};
    }

    auto channelIndex  = info[0].As<Napi::Number>().Int32Value();
    auto rawTransforms = info[1].As<Napi::Array>();

    auto& channels = instance_data->caspar_server->get_channels();

    if (channelIndex < 1 || channelIndex > channels->size()) {
        Napi::Error::New(env, "Channel index is out of bounds").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto ch = channels->at(channelIndex - 1);

    std::vector<caspar::core::stage::transform_tuple_t> transforms;
    transforms.reserve(rawTransforms.Length());
    for (int i = 0; i < rawTransforms.Length(); i++) {
        auto entry = rawTransforms.Get(i).As<Napi::Object>();

        auto layerIndex = entry.Get("layerIndex").As<Napi::Number>().Int32Value();
        auto fragment   = entry.Get("fragment").As<Napi::Object>();
        auto duration   = entry.Get("duration").As<Napi::Number>().Int32Value();
        auto tween      = entry.Get("tween").As<Napi::String>().Utf8Value();

        auto updater = createMixerTransform(env, fragment);
        transforms.emplace_back(layerIndex, updater, duration, caspar::u16(tween));
    }

    auto prom    = promiseFuncWrapper<std::string>(env);
    auto resolve = std::get<1>(prom);
    auto reject  = std::get<2>(prom);

    ch->tmp_executor_->begin_invoke([resolve, reject, ch, transforms] {
        try {
            ch->stage().apply_transforms(transforms).get();

            resolve("");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            reject("Internal error");
        }
    });

    return std::get<0>(prom);
}