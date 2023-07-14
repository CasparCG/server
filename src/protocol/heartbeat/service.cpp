/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "service.h"

#include <utility>

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
                }

                ~impl()
                {
                    if (!enabled_) return;

                    abort_request_ = true;
                    thread_.join();
                }

                void send_heartbeat() {
                    boost::system::error_code err;
                    socket_.send_to(boost::asio::buffer(message_), remote_endpoint_, 0, err);

                    if (err) {
                        CASPAR_LOG(error) << "Error sending heartbeat: " << err.message();
                    }
                }

                void set_name(const std::string& name) {
                    message_ = "CG " + name;
                }

                void enable(const configuration& config) {
                    if (enabled_) return;
                    enabled_ = true;

                    address_v4 address = calculateBroadcastAddress();
                    if (!config.host.empty()) address = address_v4::from_string(config.host);

                    remote_endpoint_ = udp::endpoint(address, config.port);
                    delay_ = config.delay;

                    thread_ = std::thread([this] {
                        try {
                            auto last_send = std::chrono::system_clock::now();
                            while (!abort_request_) {
                                auto now = std::chrono::system_clock::now();
                                std::chrono::duration<double> elapsed_seconds = now - last_send;

                                if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count() > delay_) {
                                    send_heartbeat();
                                    last_send = now;
                                }
                            }
                        } catch (...) {
                            CASPAR_LOG_CURRENT_EXCEPTION();
                        }
                    });
                }

              private:
                std::thread       thread_;
                std::atomic<bool> abort_request_{false};

                std::shared_ptr<boost::asio::io_context> service_;
                udp::socket socket_;

                std::string message_;
                bool enabled_ = false;

                udp::endpoint remote_endpoint_;
                int delay_ = 1000;

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

            void service::set_name(const std::string& name) {
                    impl_->set_name(name);
            }

            void service::enable(const configuration& config) {
                    impl_->enable(config);
            }

            service::~service() {

            }
        }
    }
}
