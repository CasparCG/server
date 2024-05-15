#include "napi.h"

#include <common/forward.h>
#include <common/memory.h>

Napi::Value AddConsumer(const Napi::CallbackInfo& info);

Napi::Value RemoveConsumerByPort(const Napi::CallbackInfo& info);
Napi::Value RemoveConsumerByParams(const Napi::CallbackInfo& info);
