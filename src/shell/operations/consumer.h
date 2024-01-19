#include "napi.h"

#include <common/forward.h>
#include <common/memory.h>

FORWARD2(caspar, core, class frame_producer)

Napi::Value AddConsumer(const Napi::CallbackInfo& info);

Napi::Value RemoveConsumerByPort(const Napi::CallbackInfo& info);
Napi::Value RemoveConsumerByParams(const Napi::CallbackInfo& info);
