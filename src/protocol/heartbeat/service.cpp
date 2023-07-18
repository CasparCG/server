/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "service.h"

using namespace boost::asio::ip;

namespace caspar {
    namespace protocol {
        namespace heartbeat {
            struct service::impl {
              public:
                explicit impl(std::shared_ptr<boost::asio::io_service> service)
                    : service_(std::move(service)),
                    socket_(*service_, udp::v4())
                {
                    socket_.set_option(udp::socket::reuse_address(true));
                    socket_.set_option(udp::socket::broadcast(true));

                    thread_ = std::thread([this] {
                        try {
                            auto last_send = std::chrono::system_clock::now();
                            while (!abort_request_) {
                                auto now = std::chrono::system_clock::now();
                                std::chrono::duration<double> elapsed_seconds = now - last_send;

                                if (elapsed_seconds.count() > 1) {
                                    send_heartbeat();
                                    last_send = now;
                                }
                            }
                        } catch (...) {
                            CASPAR_LOG_CURRENT_EXCEPTION();
                        }
                    });
                }

                ~impl()
                {
                    abort_request_ = true;
                    thread_.join();
                }

                void send_heartbeat() {
                    auto remote_endpoint = udp::endpoint(calculateBroadcastAddress(), 3468);
                    std::string message = "CG " + name_;

                    boost::system::error_code err;
                    socket_.send_to(boost::asio::buffer(message), remote_endpoint, 0, err);

                    if (err) {
                        CASPAR_LOG(error) << "Error sending heartbeat: " << err.message();
                    }
                }

                void set_name(std::string name) {
                    name_ = name;
                }

              private:
                std::thread       thread_;
                std::atomic<bool> abort_request_{false};

                std::shared_ptr<boost::asio::io_context> service_;
                udp::socket socket_;

                std::string name_;

                static address_v4 calculateBroadcastAddress() {
                    boost::asio::io_context ioContext;
                    udp::resolver resolver(ioContext);

                    udp::resolver::query query(udp::v4(), host_name(), "");
                    udp::resolver::iterator it = resolver.resolve(query);

                    while (it != udp::resolver::iterator()) {
                        udp::endpoint endpoint = *it++;
                        if (endpoint.address().is_v4()) {
                            address_v4 address = endpoint.address().to_v4();
                            address_v4 broadcast = address_v4::broadcast(address, address_v4::netmask(address));
                            return broadcast;
                        }
                    }

                    // Return default broadcast address
                    return address_v4::broadcast();
                }
            };

            service::service(std::shared_ptr<boost::asio::io_service> service) : impl_(new impl(std::move(service))) {

            }

            void service::send_heartbeat() {
                    impl_->send_heartbeat();
            }

            void service::set_name(std::string name) {
                    impl_->set_name(name);
            }

            service::~service() {

            }
        }
    }
}
