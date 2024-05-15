
#include "./channel_state.h"
#include "./conv.h"

#include <core/monitor/monitor.h>

namespace channel_state {

struct channel_osc_state
{
    const int                          channel_index;
    const caspar::core::monitor::state state;

    explicit channel_osc_state(const int channel_index, const caspar::core::monitor::state& state)
        : channel_index(channel_index)
        , state(state)
    {
    }
};

using Context  = void;
using DataType = channel_osc_state;
void CallJs(Napi::Env env, Napi::Function callback, Context* context, DataType* data);
using TSFN              = Napi::TypedThreadSafeFunction<Context, DataType, CallJs>;
using FinalizerDataType = void;

// Transform native data into JS data, passing it to the provided
// `callback` -- the TSFN's JavaScript function.
void CallJs(Napi::Env env, Napi::Function callback, Context* context, DataType* data)
{
    // Is the JavaScript environment still available to call into, eg. the TSFN is not aborted
    if (env != nullptr) {
        // On Node-API 5+, the `callback` parameter is optional; however, this example
        // does ensure a callback is provided.
        if (callback != nullptr && data != nullptr) {
            Napi::Object state   = Napi::Object::New(env);
            bool         success = MonitorStateToNapiObject(env, data->state, state);

            callback.Call({Napi::Number::New(env, data->channel_index), success ? std::move(state) : env.Null()});
        }
    }
    if (data != nullptr) {
        // We're finished with the data.
        delete data;
    }
}

class channel_state_emitter_impl
    : public caspar::channel_state_emitter
    , public std::enable_shared_from_this<channel_state_emitter_impl>
{
  public:
    void send_state(int channel_index, const caspar::core::monitor::state& state)
    {
        auto state_ptr = new channel_osc_state(channel_index, state);

        napi_status status = tsfn.NonBlockingCall(state_ptr);
        if (status != napi_ok) {
            // TODO: Handle error
        }
    }

    void init(const Napi::Env& env, Napi::Function callback)
    {
        std::shared_ptr<channel_state_emitter_impl> self = shared_from_this();

        tsfn = TSFN::New(env,
                         callback,               // JavaScript function called asynchronously
                         "Channel State Sender", // Name
                         0,                      // Unlimited queue
                         1,                      // Only one thread will use this initially
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

} // namespace channel_state

std::shared_ptr<caspar::channel_state_emitter> create_channel_state_emitter(const Napi::Env& env,
                                                                            Napi::Function   callback)
{
    auto sender = std::make_shared<channel_state::channel_state_emitter_impl>();
    sender->init(env, callback);

    // Re-wrap the pointer to call release instead of destructor upon release
    return std::shared_ptr<caspar::channel_state_emitter>(sender.get(), [sender](caspar::channel_state_emitter*) {
        // Trigger a release of the TSFn
        sender->release();
    });
}