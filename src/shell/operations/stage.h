#include "napi.h"

#include "./base.h"

Napi::Value StagePlay(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StagePreview(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageLoad(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StagePause(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageResume(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageStop(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageClear(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageCall(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value StageSwapChannel(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);
Napi::Value StageSwapLayer(const Napi::CallbackInfo& info, CasparCgInstanceData* instance_data);

Napi::Value GetLayerMixerProperties(const Napi::CallbackInfo& info);