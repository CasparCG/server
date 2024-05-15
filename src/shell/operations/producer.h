#include "napi.h"

#include <common/forward.h>
#include <common/memory.h>

FORWARD2(caspar, core, class frame_producer)

class NodeUnusedProducer : public Napi::ObjectWrap<NodeUnusedProducer>
{
  public:
    static Napi::FunctionReference* Init(Napi::Env env, Napi::Object exports);

    static caspar::spl::shared_ptr<caspar::core::frame_producer> TakeProducer(const Napi::Object& object);

    NodeUnusedProducer(const Napi::CallbackInfo& info);

    Napi::Value IsValid(const Napi::CallbackInfo& info);

    caspar::spl::shared_ptr<caspar::core::frame_producer> _producer;
};

Napi::Value CreateProducer(const Napi::CallbackInfo& info);

Napi::Value CreateStingTransition(const Napi::CallbackInfo& info);

Napi::Value CreateBasicTransition(const Napi::CallbackInfo& info);
