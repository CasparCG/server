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
            class service {
              public:
                explicit service(std::shared_ptr<boost::asio::io_service> service);
                ~service();

                void set_name(std::string name);

              private:
                struct impl;
                spl::shared_ptr<impl> impl_;

                void send_heartbeat();
            };
        }
    }
}
