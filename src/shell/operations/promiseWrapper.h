// Based on https://github.com/SurienDG/NAPI-Thread-Safe-Promise/blob/master/src/promiseWrapper.h
#include <functional>
#include <memory>
#include <napi.h>

// promise function types
template <typename T>
using resolveFunc = std::function<void(const T args)>;
using rejectFunc  = std::function<void(const std::string args)>;

template <typename T>
struct tfsnContext
{
    Napi::Promise::Deferred  deferred;
    T                        data;
    std::string              error;
    bool                     resolve;
    bool                     called;
    Napi::ThreadSafeFunction tsfn;
    tfsnContext(Napi::Env env)
        : deferred{Napi::Promise::Deferred::New(env)}
        , resolve{false}
        , called{false} {};
};

// this will give the promise function (promFunc) the resolve and reject
// functions the promise function can then be used to do the main work
template <typename T, typename Fn>
std::tuple<Napi::Promise, resolveFunc<T>, rejectFunc> promiseFuncWrapper2(const Napi::Env env, const Fn func)
{
    std::shared_ptr<tfsnContext<T>> context = std::make_shared<tfsnContext<T>>(env);
    std::shared_ptr<std::mutex>     mu      = std::make_shared<std::mutex>();
    context->tsfn =
        Napi::ThreadSafeFunction::New(env,
                                      Napi::Function::New(env, [](const Napi::CallbackInfo& info) {}),
                                      "TSFN",
                                      0,
                                      1,
                                      [context, mu, func](Napi::Env env) {
                                          std::lock_guard<std::mutex> lock(*mu);
                                          if (context->resolve) {
                                              context->deferred.Resolve(func(env, context->data));
                                          } else {
                                              context->deferred.Reject(Napi::Error::New(env, context->error).Value());
                                          }
                                      });

    // create resolve function
    resolveFunc<T> resolve = [context, mu](const T msg) {
        std::lock_guard<std::mutex> lock(*mu);
        context->data    = msg;
        context->resolve = true;
        if (!context->called) {
            context->called = true;
            context->tsfn.Release();
        }
    };
    // create reject function
    rejectFunc reject = [context, mu](const std::string& msg) {
        std::lock_guard<std::mutex> lock(*mu);
        context->error   = msg;
        context->resolve = false;
        if (!context->called) {
            context->called = true;
            context->tsfn.Release();
        }
    };

    return std::make_tuple(context->deferred.Promise(), resolve, reject);
}

// this will give the promise function (promFunc) the resolve and reject
// functions the promise function can then be used to do the main work
template <typename T>
std::tuple<Napi::Promise, resolveFunc<T>, rejectFunc> promiseFuncWrapper(const Napi::Env env)
{
    auto convert = [](const Napi::Env& env, const T& data) {
        if constexpr (std::is_same<void*, T>::value)
            return env.Null();
        else if constexpr (std::is_same<int, T>::value)
            return Napi::Number::New(env, data);
        else
            return Napi::String::New(env, data);
    };

    return promiseFuncWrapper2<T, decltype(convert)>(env, convert);
}