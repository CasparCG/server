#include "napi.h"

#include "./base.h"

Napi::Value CgAdd(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgPlay(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgNext(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgRemove(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgInvoke(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value CgUpdate(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
