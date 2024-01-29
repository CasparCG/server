#include "napi.h"

#include "./base.h"

Napi::Value CgPlay(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgNext(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgRemove(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);