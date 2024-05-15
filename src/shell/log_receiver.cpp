
#include "./log_receiver.h"

namespace log_reciever {

struct log_message
{
    int64_t           timestamp;
    const std::string message;
    const std::string level;

    explicit log_message(int64_t timestamp, const std::string& message, const std::string& level)
        : timestamp(timestamp)
        , message(message)
        , level(level)
    {
    }
};

using Context  = void;
using DataType = log_message;
void CallJs(Napi::Env env, Napi::Function callback, Context* context, DataType* data);
using TSFN              = Napi::TypedThreadSafeFunction<Context, DataType, CallJs>;
using FinalizerDataType = void;

// Transform native data into JS data, passing it to the provided
// `callback` -- the TSFN's JavaScript function.
void CallJs(Napi::Env env, Napi::Function callback, Context* context, DataType* data)
{
    // Is the JavaScript environment still available to call into, eg. the TSFN is not aborted
    if (env != nullptr) {
        if (callback != nullptr && data != nullptr) {
            callback.Call({Napi::Number::New(env, data->timestamp),
                           Napi::String::New(env, data->message),
                           Napi::String::New(env, data->level)});
        }
    }
    if (data != nullptr) {
        // We're finished with the data.
        delete data;
    }
}

class log_receiver_emitter_impl
    : public caspar::log::log_receiver_emitter
    , public std::enable_shared_from_this<log_receiver_emitter_impl>
{
  public:
    void send_log(int64_t timestamp, const std::string& message, const std::string& level)
    {
        auto state_ptr = new log_message(timestamp, message, level);

        napi_status status = tsfn.NonBlockingCall(state_ptr);
        if (status != napi_ok) {
            // TODO: Handle error
        }
    }

    void init(const Napi::Env& env, Napi::Function callback)
    {
        std::shared_ptr<log_receiver_emitter_impl> self = shared_from_this();

        tsfn = TSFN::New(env,
                         callback,             // JavaScript function called asynchronously
                         "Server Log Emitter", // Name
                         0,                    // Unlimited queue
                         1,                    // Only one thread will use this initially
                         nullptr,
                         [self](Napi::Env,
                                FinalizerDataType*,
                                Context* ctx) { // Finalizer used to clean threads up
                             // self will be freed when falling out of scope
                         });
    }

    void release() { tsfn.Release(); }

  private:
    TSFN tsfn;
};

} // namespace log_reciever

std::shared_ptr<caspar::log::log_receiver_emitter> create_log_receiver_emitter(const Napi::Env& env,
                                                                               Napi::Function   callback)
{
    auto sender = std::make_shared<log_reciever::log_receiver_emitter_impl>();
    sender->init(env, callback);

    // Re-wrap the pointer to call release instead of destructor upon release
    return std::shared_ptr<caspar::log::log_receiver_emitter>(sender.get(),
                                                              [sender](caspar::log::log_receiver_emitter*) {
                                                                  // Trigger a release of the TSFn
                                                                  sender->release();
                                                              });
}