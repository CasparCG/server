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

#include <chrono>
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

// OSC time tags are NTP format: seconds since 1900-01-01 in the high 32 bits, fractional
// seconds in the low 32 bits.
uint64_t osc_time_tag()
{
    using namespace std::chrono;

    constexpr uint64_t ntp_epoch_offset = 2208988800ULL;

    auto since_epoch = system_clock::now().time_since_epoch();
    auto secs        = duration_cast<seconds>(since_epoch);
    auto frac        = duration_cast<duration<double>>(since_epoch - secs).count();

    return ((static_cast<uint64_t>(secs.count()) + ntp_epoch_offset) << 32) |
           static_cast<uint64_t>(frac * 4294967296.0);
}

constexpr unsigned long max_bundle_size = 2048;

void write_osc_message(::osc::OutboundPacketStream& stream, const std::string& address, const core::monitor::vector_t& args)
{
    stream << ::osc::BeginMessage(address.c_str());

    param_visitor<::osc::OutboundPacketStream> visitor(stream);
    for (const auto& element : args) {
        boost::apply_visitor(visitor, element);
    }

    stream << ::osc::EndMessage;
}

struct client::impl : public spl::enable_shared_from_this<client::impl>
{
    std::shared_ptr<boost::asio::io_context> io_context_;
    udp::socket                              socket_;
    std::map<udp::endpoint, int>             reference_counts_by_endpoint_;
    std::vector<char>                        buffer_;
    std::vector<char>                        probe_buffer_;

    std::mutex              mutex_;
    std::condition_variable cond_;
    core::monitor::state    bundle_;
    uint64_t                bundle_time_    = 0;
    bool                    bundle_pending_ = false;

    std::atomic<bool> abort_request_{false};
    std::thread       thread_;

  public:
    impl(std::shared_ptr<boost::asio::io_context> io_context)
        : io_context_(std::move(io_context))
        , socket_(*io_context_, udp::v4())
        , buffer_(1000000)
        , probe_buffer_(65536)
    {
        thread_ = std::thread([this] {
            try {
                while (!abort_request_) {
                    core::monitor::state       bundle;
                    uint64_t                   bundle_time;
                    std::vector<udp::endpoint> endpoints;

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [&] { return bundle_pending_ || abort_request_; });

                        if (abort_request_) {
                            return;
                        }

                        bundle          = std::move(bundle_);
                        bundle_         = core::monitor::state{};
                        bundle_time     = bundle_time_;
                        bundle_pending_ = false;

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

                        // Measure each message's real encoded size before adding it, so one
                        // that wouldn't fit is deferred to the next bundle instead of silently
                        // overflowing this one. The first message of a bundle is always added
                        // regardless of size, so an oversized message can't stall the loop.
                        bool has_message = false;
                        while (it != std::end(bundle)) {
                            ::osc::OutboundPacketStream probe(probe_buffer_.data(),
                                                              static_cast<unsigned long>(probe_buffer_.size()));
                            write_osc_message(probe, it->first, it->second);

                            if (has_message && o.Size() + probe.Size() > max_bundle_size) {
                                break;
                            }

                            write_osc_message(o, it->first, it->second);
                            has_message = true;
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

    // shared_ptr<void> with a null pointee and a cleanup deleter, so this composes with the
    // generic protocol_strategy::add_lifecycle_bound_object(shared_ptr<void>) lifetime binding
    // used for every other per-connection resource (see lock_container.cpp for the same pattern).
    std::shared_ptr<void> get_subscription_token(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (++reference_counts_by_endpoint_[endpoint] == 1) {
            // New subscriber: let change_gate-backed state know it may need to resend.
            core::monitor::bump_subscription_epoch();
        }

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

            // Merge rather than replace, so a pending bundle isn't lost before the sender
            // thread drains it.
            for (auto& p : state) {
                bundle_[p.first] = p.second;
            }

            if (!bundle_pending_) {
                bundle_time_    = osc_time_tag();
                bundle_pending_ = true;
            }
        }
        cond_.notify_all();
    }
};

client::client(std::shared_ptr<boost::asio::io_context> io_context)
    : impl_(new impl(std::move(io_context)))
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
