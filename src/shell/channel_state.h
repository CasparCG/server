#include "napi.h"

#include "./server.h"

std::shared_ptr<caspar::channel_state_emitter> create_channel_state_emitter(const Napi::Env& env,
                                                                            Napi::Function   callback);