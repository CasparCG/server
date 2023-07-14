/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
*/

#include "dmx.h"

#define WIN32_LEAN_AND_MEAN

#include "consumer/dmx_consumer.h"

#include <core/consumer/frame_consumer.h>

#include <common/utf.h>

namespace caspar {
    namespace dmx {

        void init(const core::module_dependencies& dependencies)
        {
            // TODO: Initialise DMX connection

            dependencies.consumer_registry->register_consumer_factory(L"DMX Consumer", create_consumer);
        }

        void uninit() {
           // TODO: Remove DMX connection
        }

    }
} // namespace caspar::dmx
