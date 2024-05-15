
#include "./base.h"

#include <common/os/filesystem.h>
#include <common/utf.h>

#include <optional>

class FindCaseInsensitiveWorker : public PromiseAsyncWorker
{
  public:
    FindCaseInsensitiveWorker(const Napi::Env& env, const std::string& filepath)
        : PromiseAsyncWorker(env)
        , filepath(filepath)
    {
    }

    ~FindCaseInsensitiveWorker() {}

    // This code will be executed on the worker thread
    void Execute() override { result = caspar::find_case_insensitive(caspar::u16(filepath)); }

    Napi::Value GetPromiseResult(const Napi::Env& env) override
    {
        if (result.has_value()) {
            return Napi::String::New(env, caspar::u8(result.value()));
        } else {
            return env.Null();
        }
    }

  private:
    const std::string           filepath;
    std::optional<std::wstring> result;
};

Napi::Value FindCaseInsensitive(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::Error::New(env, "Expected file path").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto filepath = info[0].As<Napi::String>();

    return (new FindCaseInsensitiveWorker(env, filepath.Utf8Value()))->QueueAndRun();
}