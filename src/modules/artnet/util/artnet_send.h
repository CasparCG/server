/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#pragma once

#include <boost/asio.hpp>

#include <string>
#include <vector>

namespace caspar {
    namespace artnet {

        void send_dmx_data(unsigned short port, const std::wstring& host, int universe, const std::uint8_t* data, size_t length);
        void send_dmx_data(unsigned short port, const std::wstring& host, int universe, std::vector<uint8_t> data);
    }
} // namespace caspar::artnet
