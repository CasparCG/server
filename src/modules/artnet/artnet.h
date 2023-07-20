/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */


#pragma once

#include <core/module_dependencies.h>

namespace caspar {
    namespace artnet {

        void init(const core::module_dependencies& dependencies);
        void uninit();

    }
} // namespace caspar::artnet

