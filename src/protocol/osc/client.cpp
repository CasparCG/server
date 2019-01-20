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

#include "oscpack/OscOutboundPacketStream.h"

#include <common/endian.h>
#include <common/utf.h>

#include <core/monitor/monitor.h>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

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

    std::mutex              mutex_;
    std::condition_variable cond_;
    core::monitor::state    bundle_;
    uint64_t                bundle_time_ = 0;

    uint64_t time_ = 0;

    std::atomic<bool> abort_request_{false};
    std::thread       thread_;

  public:
    impl(std::shared_ptr<boost::asio::io_service> service)
        : service_(std::move(service))
        , socket_(*service_, udp::v4())
        , buffer_(1000000)
    {
        thread_ = std::thread([=] {
            try {
                while (!abort_request_) {
                    core::monitor::state       bundle;
                    uint64_t                   bundle_time;
                    std::vector<udp::endpoint> endpoints;

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [&] { return bundle_time_ > 0 || abort_request_; });

                        if (abort_request_) {
                            return;
                        }

                        bundle       = std::move(bundle_);
                        bundle_time  = bundle_time_;
                        bundle_time_ = 0;

                        for (auto& p : reference_counts_by_endpoint_) {
                            endpoints.push_back(p.first);
                        }
                    }

                    if (endpoints.empty()) {
                        continue;
                    }

                    auto it = std::begin(bundle);

                    while (it != std::end(bundle)) {
                        ::osc::OutboundPacketStream o(reinterpret_cast<char*>(buffer_.data()),
                                                      static_cast<unsigned long>(buffer_.size()));

                        o << ::osc::BeginBundle(bundle_time);

                        // TODO (fix): < 2048 is a hack. Properly calculate if messages will fit.
                        while (it != std::end(bundle) && o.Size() < 2048) {
                            o << ::osc::BeginMessage(it->first.c_str());

                            param_visitor<decltype(o)> param_visitor(o);
                            for (const auto& element : it->second) {
                                boost::apply_visitor(param_visitor, element);
                            }

                            o << ::osc::EndMessage;

                            ++it;
                        }

                        o << ::osc::EndBundle;

                        boost::system::error_code ec;
                        for (const auto& endpoint : endpoints) {
                            socket_.send_to(boost::asio::buffer(o.Data(), o.Size()), endpoint, 0, ec);
                        }
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
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // TODO: time_++ is a hack. Use proper channel time.
            bundle_time_ = time_++;
            bundle_      = state;
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

void client::send(core::monitor::state state) { impl_->send(state); }

}}} // namespace caspar::protocol::osc
