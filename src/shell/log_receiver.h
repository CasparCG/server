#include "napi.h"

#include "../common/log.h"

std::shared_ptr<caspar::log::log_receiver_emitter> create_log_receiver_emitter(const Napi::Env& env,
                                                                               Napi::Function   callback);