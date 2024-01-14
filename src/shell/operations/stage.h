#include "napi.h"

#include "./base.h"

Napi::Value StageClear(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageCall(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageSwapChannel(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageSwapLayer(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
