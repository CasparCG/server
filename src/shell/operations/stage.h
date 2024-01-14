#include "napi.h"

#include "./base.h"

Napi::Value StagePause(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageResume(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageClear(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageCall(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageSwapChannel(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageSwapLayer(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
