/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#pragma once

#include <boost/asio.hpp>

namespace caspar {
    namespace protocol {
        namespace heartbeat {
            struct configuration {
                std::string host;
                unsigned short port = 3468;

                int delay = 1000;
            };

            class service {
              public:
                explicit service(std::shared_ptr<boost::asio::io_service> service);
                ~service();

                void set_name(const std::string& name);
                void enable(const configuration& config);

                void send_heartbeat();

              private:
                struct impl;
                spl::shared_ptr<impl> impl_;
            };
        }
    }
}
