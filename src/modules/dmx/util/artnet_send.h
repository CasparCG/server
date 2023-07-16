/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#pragma once

#include "boost/asio.hpp"

#include <string>
#include <vector>

namespace caspar {
    namespace dmx {

        void send_dmx_data(int port, std::string host, int universe, std::vector<uint8_t> data);
    }
} // namespace caspar::dmx
