/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "artnet_send.h"

#include <locale>
#include <string>
#include <vector>

#include <codecvt>

namespace caspar {
    namespace dmx {

        void send_dmx_data(unsigned short port, const std::wstring& host, int universe, const std::uint8_t* data, size_t length) {
            std::uint8_t hUni = (universe >> 8) & 0xff;
            std::uint8_t lUni = universe & 0xff;

            std::uint8_t hLen = (length >> 8) & 0xff;
            std::uint8_t lLen = (length & 0xff);

            std::uint8_t header[] = {65, 114, 116, 45, 78, 101, 116, 0, 0, 80, 0, 14, 0, 0, lUni, hUni, hLen, lLen};
            std::uint8_t buffer[18 + 512];

            for (int i = 0; i < 18 + 512; i++) {
                if (i < 18) {
                    buffer[i] = header[i];
                    continue;
                }

                if (i - 18 < length) {
                    buffer[i] = data[i - 18];
                    continue;
                }

                buffer[i] = 0;
            }

            boost::asio::io_service io_service;
            boost::asio::ip::udp::socket socket(io_service);
            boost::asio::ip::udp::endpoint remote_endpoint;

            socket.open(boost::asio::ip::udp::v4());

            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
            std::string host_ = converterX.to_bytes(host);

            remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(host_), port);

            boost::system::error_code err;
            socket.send_to(boost::asio::buffer(buffer), remote_endpoint, 0, err);

            socket.close();
        }

        void send_dmx_data(unsigned short port, const std::wstring& host, int universe, std::vector<std::uint8_t> data) {
            size_t length = data.size();

            std::uint8_t buffer[512];
            memset(buffer, 0, 512);

            for (int i = 0; i < length; i++) buffer[i] = data[i];

            send_dmx_data(port, host, universe, buffer, length);
        }
    }
} // namespace caspar::dmx
