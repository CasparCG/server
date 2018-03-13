/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "../StdAfx.h"

#include "client.h"

#include "oscpack/OscHostEndianness.h"
#include "oscpack/OscOutboundPacketStream.h"

#include <common/endian.h>
#include <common/except.h>
#include <common/utf.h>

#include <core/monitor/monitor.h>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>

using namespace boost::asio::ip;

namespace caspar { namespace protocol { namespace osc {

template <typename T>
struct param_visitor : public boost::static_visitor<void>
{
    T& o;

    param_visitor(T& o)
        : o(o)
    {
    }

    void operator()(const bool value) { o << value; }
    void operator()(const int32_t value) { o << static_cast<int32_t>(value); }
    void operator()(const uint32_t value) { o << static_cast<int64_t>(value); }
    void operator()(const int64_t value) { o << static_cast<int64_t>(value); }
    void operator()(const uint64_t value) { o << static_cast<int64_t>(value); }
    void operator()(const float value) { o << value; }
    void operator()(const double value) { o << static_cast<float>(value); }
    void operator()(const std::string& value) { o << value.c_str(); }
    void operator()(const std::wstring& value) { o << u8(value).c_str(); }
};

struct client::impl : public spl::enable_shared_from_this<client::impl>
{
    std::shared_ptr<boost::asio::io_context> service_;
    udp::socket                              socket_;
    std::map<udp::endpoint, int>             reference_counts_by_endpoint_;
    std::vector<char>                        buffer_;

    std::mutex                mutex_;
    std::condition_variable   cond_;
    boost::optional<core::monitor::data_map_t> bundle_opt_;
    std::atomic<bool>         abort_request_{ false };
    std::thread               thread_;

  public:
    impl(std::shared_ptr<boost::asio::io_service> service)
        : service_(std::move(service))
        , socket_(*service_, udp::v4())
        // TODO (fix) Split into smaller packets.
        , buffer_(1000000)
    {
        thread_ = std::thread([=] {
            try {
                while (!abort_request_) {
                    core::monitor::data_map_t bundle;
                    std::vector<udp::endpoint> endpoints;

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [&] { return bundle_opt_ || abort_request_;  });

                        if (abort_request_) {
                            return;
                        }

                        bundle = std::move(*bundle_opt_);
                        bundle_opt_.reset();

                        for (auto& p : reference_counts_by_endpoint_) {
                            endpoints.push_back(p.first);
                        }
                    }

                    if (endpoints.empty()) {
                        continue;
                    }

                    ::osc::OutboundPacketStream o(reinterpret_cast<char*>(buffer_.data()),
                                                  static_cast<unsigned long>(buffer_.size()));

                    o << ::osc::BeginBundle();

                    for (auto& p : bundle) {
                        o << ::osc::BeginMessage(p.first.c_str());

                        param_visitor<decltype(o)> param_visitor(o);
                        for (const auto& element : p.second) {
                            boost::apply_visitor(param_visitor, element);
                        }

                        o << ::osc::EndMessage;
                    }

                    o << ::osc::EndBundle;

                    boost::system::error_code ec;
                    for (const auto& endpoint : endpoints) {
                        socket_.send_to(boost::asio::buffer(o.Data(), o.Size()), endpoint, 0, ec);
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
        cond_.notify_all();
        thread_.join();
    }

    // TODO (refactor) This is wierd...
    std::shared_ptr<void> get_subscription_token(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        ++reference_counts_by_endpoint_[endpoint];

        std::weak_ptr<impl> weak_self = shared_from_this();

        return std::shared_ptr<void>(nullptr, [weak_self, endpoint](void*) {
            auto strong = weak_self.lock();

            if (!strong)
                return;

            auto& self = *strong;

            std::lock_guard<std::mutex> lock(self.mutex_);

            int reference_count_after = --self.reference_counts_by_endpoint_[endpoint];

            if (reference_count_after == 0) {
                self.reference_counts_by_endpoint_.erase(endpoint);
            }
        });
    }

    void send(const core::monitor::state& state)
    {
        auto bundle = state.get();
        if (bundle.empty()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            bundle_opt_ = std::move(bundle);
        }
        cond_.notify_all();
    }
};

client::client(std::shared_ptr<boost::asio::io_context> service)
    : impl_(new impl(std::move(service)))
{
}

client::client(client&& other)
    : impl_(std::move(other.impl_))
{
}

client& client::operator=(client&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}

client::~client() {}

std::shared_ptr<void> client::get_subscription_token(const boost::asio::ip::udp::endpoint& endpoint)
{
    return impl_->get_subscription_token(endpoint);
}

void client::send(const core::monitor::state& state) { impl_->send(state); }

}}} // namespace caspar::protocol::osc
