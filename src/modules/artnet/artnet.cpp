/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
*/

#include "artnet.h"

#define WIN32_LEAN_AND_MEAN

#include "consumer/artnet_consumer.h"

#include <core/consumer/frame_consumer.h>

#include <common/utf.h>

namespace caspar {
    namespace artnet {

        void init(const core::module_dependencies& dependencies)
        {
            dependencies.consumer_registry->register_preconfigured_consumer_factory(L"artnet", create_preconfigured_consumer);
        }

    }
} // namespace caspar::artnet
