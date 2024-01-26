#include "./conv.h"

#include <iostream>

bool NapiObjectToBoostPropertyTree(const Napi::Env& env, const Napi::Object& object, boost::property_tree::wptree& tree)
{
    auto property_names = object.GetPropertyNames();
    for (size_t i = 0; i < property_names.Length(); i++) {
        auto name = property_names.Get(i);
        if (!name.IsString()) {
            Napi::Error::New(env, "Expected keys to be strings").ThrowAsJavaScriptException();
            return false;
        }
        auto nameStr = caspar::u16(name.As<Napi::String>().Utf8Value());

        auto value = object.Get(name);
        if (value.IsNumber()) {
            auto valueNumber = value.As<Napi::Number>();
            tree.put(nameStr, valueNumber.Int32Value());
            // TODO - floats?
        } else if (value.IsString()) {
            auto valueStr = value.As<Napi::String>();
            tree.put(nameStr, caspar::u16(valueStr.Utf8Value()));
        } else if (value.IsBoolean()) {
            auto valueBool = value.As<Napi::Boolean>();
            tree.put(nameStr, valueBool.Value());
        } else if (value.IsObject()) {
            auto valueObj = value.As<Napi::Object>();

            boost::property_tree::wptree child_tree;
            if (!NapiObjectToBoostPropertyTree(env, valueObj, child_tree)) {
                return false;
            }

            tree.put_child(nameStr, child_tree);
        } else {
            Napi::Error::New(env, "Unsupported value type").ThrowAsJavaScriptException();
            return false;
        }
    }

    return true;
}

void NapiArrayToStringVector(const Napi::Env& env, const Napi::Array& array, std::vector<std::wstring>& result)
{
    result.reserve(array.Length());
    for (size_t i = 0; i < array.Length(); i++) {
        auto str = static_cast<Napi::Value>(array[i]).ToString();
        result.emplace_back(caspar::u16(str.Utf8Value()));
    }
}

struct param_visitor : public boost::static_visitor<void>
{
    const Napi::Env& env;
    Napi::Array&     o;
    uint32_t         index;

    param_visitor(const Napi::Env& env, Napi::Array& o)
        : env(env)
        , o(o)
        , index(0)
    {
    }

    void operator()(const bool value) { o.Set(index++, Napi::Boolean::New(env, value)); }
    void operator()(const int32_t value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const uint32_t value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const int64_t value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const uint64_t value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const float value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const double value) { o.Set(index++, Napi::Number::New(env, value)); }
    void operator()(const std::string& value) { o.Set(index++, Napi::String::New(env, value)); }
    void operator()(const std::wstring& value) { o.Set(index++, Napi::String::New(env, caspar::u8(value))); }
};

bool MonitorStateToNapiObject(const Napi::Env& env, const caspar::core::monitor::state& state, Napi::Object& object)
{
    for (auto&& item : state) {
        Napi::Array val = Napi::Array::New(env, item.second.size());

        param_visitor param_visitor(env, val);
        for (const auto& element : item.second) {
            boost::apply_visitor(param_visitor, element);
        }

        object.Set(item.first, val);
    }
    return true;
}