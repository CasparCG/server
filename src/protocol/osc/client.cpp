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
#include <boost/container/small_vector.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace boost::asio::ip;

namespace caspar { namespace protocol { namespace osc {

// http://stackoverflow.com/questions/14993000/the-most-reliable-and-efficient-udp-packet-size
const auto SAFE_DATAGRAM_SIZE = 1472;
const auto MAX_ELEMENT_SIZE   = 256;

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
    {
        o << ::osc::Blob(value.data(), static_cast<unsigned long>(value.size()));
    }
};

struct client::impl : public spl::enable_shared_from_this<client::impl>
{
    std::shared_ptr<boost::asio::io_context> service_;
    udp::socket                              socket_;
    std::mutex                               endpoints_mutex_;
    std::map<udp::endpoint, int>             reference_counts_by_endpoint_;

  public:
    impl(std::shared_ptr<boost::asio::io_service> service)
        : service_(std::move(service))
        , socket_(*service_, udp::v4())
    {
    }

    std::shared_ptr<void> get_subscription_token(const boost::asio::ip::udp::endpoint& endpoint)
    {
        {
            std::lock_guard<std::mutex> lock(endpoints_mutex_);
            ++reference_counts_by_endpoint_[endpoint];
        }

        std::weak_ptr<impl> weak_self = shared_from_this();

        return std::shared_ptr<void>(nullptr, [weak_self, endpoint](void*) {
            auto self = weak_self.lock();

            if (!self) {
                return;
            }

            {
                std::lock_guard<std::mutex> lock(self->endpoints_mutex_);
                if (--self->reference_counts_by_endpoint_[endpoint] == 0) {
                    self->reference_counts_by_endpoint_.erase(endpoint);
                }
            }
        });
    }

    void send(const core::monitor::state& state)
    {
        auto elements    = state.get();
        auto elements_it = elements.begin();

        while (elements_it != elemends.end()) {
            char buffer[SAFE_DATAGRAM_SIZE];
            ::osc::OutboundPacketStream o(buffer, SAFE_DATAGRAM_SIZE);
            
            o << ::osc::BeginBundle();

            while (elements_it != elemends.end() && o.Size() < SAFE_DATAGRAM_SIZE - MAX_ELEMENT_SIZE) {
                o << ::osc::BeginMessage(msg_path.c_str());

                param_visitor<decltype(o)> param_visitor(o);
                for (const auto& data : msg_data) {
                    boost::apply_visitor(param_visitor, data);
                }

                o << ::osc::EndMessage();

                ++elements_it;
            }

            o << ::osc::EndBundle();

            {
                std::lock_guard<std::mutex> lock(endpoints_mutex_);

                boost::system::error_code ec;
                for (const auto& endpoint : endpoints) {
                    socket_.send_to(boost::asio::buffer(o.Data(), o.Size()), endpoint, 0, ec);
                }
            }
        }
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
