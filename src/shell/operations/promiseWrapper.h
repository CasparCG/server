// Based on https://github.com/SurienDG/NAPI-Thread-Safe-Promise/blob/master/src/promiseWrapper.h
#include <functional>
#include <memory>
#include <napi.h>

// promise function types
typedef std::function<void(const std::string args)> resolveFunc;
typedef std::function<void(const std::string args)> rejectFunc;

struct tfsnContext
{
    Napi::Promise::Deferred  deferred;
    std::string              data;
    bool                     resolve;
    bool                     called;
    Napi::ThreadSafeFunction tsfn;
    tfsnContext(Napi::Env env)
        : deferred{Napi::Promise::Deferred::New(env)}
        , data{""}
        , resolve{false}
        , called{false} {};
};

// this will give the promise function (promFunc) the resolve and reject
// functions the promise function can then be used to do the main work
std::tuple<Napi::Promise, resolveFunc, rejectFunc> promiseFuncWrapper(const Napi::Env env)
{
    std::shared_ptr<tfsnContext> context = std::make_shared<tfsnContext>(env);
    std::shared_ptr<std::mutex>  mu      = std::make_shared<std::mutex>();
    context->tsfn =
        Napi::ThreadSafeFunction::New(env,
                                      Napi::Function::New(env, [](const Napi::CallbackInfo& info) {}),
                                      "TSFN",
                                      0,
                                      1,
                                      [context, mu](Napi::Env env) {
                                          std::lock_guard<std::mutex> lock(*mu);
                                          if (context->resolve) {
                                              context->deferred.Resolve(Napi::String::New(env, context->data));
                                          } else {
                                              context->deferred.Reject(Napi::Error::New(env, context->data).Value());
                                          }
                                      });

    // create resolve function
    resolveFunc resolve = [context, mu](const std::string msg) {
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
        context->data    = msg;
        context->resolve = false;
        if (!context->called) {
            context->called = true;
            context->tsfn.Release();
        }
    };

    return std::make_tuple(context->deferred.Promise(), resolve, reject);
}