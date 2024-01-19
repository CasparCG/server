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
template <typename T>
std::tuple<Napi::Promise, resolveFunc<T>, rejectFunc> promiseFuncWrapper(const Napi::Env env)
{
    std::shared_ptr<tfsnContext<T>> context = std::make_shared<tfsnContext<T>>(env);
    std::shared_ptr<std::mutex>     mu      = std::make_shared<std::mutex>();
    context->tsfn =
        Napi::ThreadSafeFunction::New(env,
                                      Napi::Function::New(env, [](const Napi::CallbackInfo& info) {}),
                                      "TSFN",
                                      0,
                                      1,
                                      [context, mu](Napi::Env env) {
                                          std::lock_guard<std::mutex> lock(*mu);
                                          if (context->resolve) {
                                              Napi::Value value;
                                              if constexpr (std::is_same<int, T>::value)
                                                  value = Napi::Number::New(env, context->data);
                                              else
                                                  value = Napi::String::New(env, context->data);

                                              context->deferred.Resolve(value);
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