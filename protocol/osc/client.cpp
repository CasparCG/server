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
#include <boost/bind.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace boost::asio::ip;

namespace caspar { namespace protocol { namespace osc {

typedef std::vector<char> byte_vector;

template <typename T>
struct param_visitor : public boost::static_visitor<void>
{
    T& o;

    param_visitor(T& o)
        : o(o)
    {
    }

    void operator()(const bool value) { o << value; }
    void operator()(const int32_t value) { o << static_cast<int64_t>(value); }
    void operator()(const uint32_t value) { o << static_cast<int64_t>(value); }
    void operator()(const int64_t value) { o << static_cast<int64_t>(value); }
    void operator()(const uint64_t value) { o << static_cast<int64_t>(value); }
    void operator()(const float value) { o << value; }
    void operator()(const double value) { o << static_cast<float>(value); }
    void operator()(const std::string& value) { o << value.c_str(); }
    void operator()(const std::wstring& value) { o << u8(value).c_str(); }
    void operator()(const std::vector<int8_t>& value)
    {
        o << ::osc::Blob(value.data(), static_cast<unsigned long>(value.size()));
    }
};

void write_osc_event(byte_vector& destination, const core::monitor::message& message, int retry_allocation_attempt = 0)
{
    static std::size_t max_size = 128;

    destination.resize(max_size);

    ::osc::OutboundPacketStream o(reinterpret_cast<char*>(destination.data()),
                                  static_cast<unsigned long>(destination.size()));

    try {
        o << ::osc::BeginMessage(message.path().c_str());

        param_visitor<decltype(o)> param_visitor(o);
        for (const auto& data : message.data())
            boost::apply_visitor(param_visitor, data);

        o << ::osc::EndMessage;
    } catch (const ::osc::OutOfBufferMemoryException& e) {
        if (retry_allocation_attempt > message.data().size())
            throw;

        max_size = e.required;
        CASPAR_LOG(trace) << L"[osc] Too small buffer for osc message. Increasing to " << max_size;
        return write_osc_event(destination, message, retry_allocation_attempt + 1);
    }

    destination.resize(o.Size());
}

byte_vector write_osc_bundle_start()
{
    byte_vector destination;
    destination.resize(16);

    ::osc::OutboundPacketStream o(reinterpret_cast<char*>(destination.data()),
                                  static_cast<unsigned long>(destination.size()));
    o << ::osc::BeginBundle();

    destination.resize(o.Size());

    return destination;
}

void write_osc_bundle_element_start(byte_vector& destination, const byte_vector& message)
{
    destination.resize(4);

    int32_t* bundle_element_size = reinterpret_cast<int32_t*>(destination.data());

#ifdef OSC_HOST_LITTLE_ENDIAN
    *bundle_element_size = swap_byte_order(static_cast<int32_t>(message.size()));
#else
    *bundle_element_size = static_cast<int32_t>(bundle.size());
#endif
}

struct client::impl
    : public spl::enable_shared_from_this<client::impl>
    , core::monitor::sink
{
    std::shared_ptr<boost::asio::io_context> service_;
    udp::socket                              socket_;
    std::mutex                               endpoints_mutex_;
    std::map<udp::endpoint, int>             reference_counts_by_endpoint_;

    std::unordered_map<std::string, byte_vector> updates_;
    std::mutex                                   updates_mutex_;
    std::condition_variable                      updates_cond_;

    std::atomic<bool> is_running_;

    std::thread thread_;

  public:
    impl(std::shared_ptr<boost::asio::io_service> service)
        : service_(std::move(service))
        , socket_(*service_, udp::v4())
        , thread_(boost::bind(&impl::run, this))
    {
    }

    ~impl()
    {
        is_running_ = false;

        updates_cond_.notify_one();

        thread_.join();
    }

    std::shared_ptr<void> get_subscription_token(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::lock_guard<std::mutex> lock(endpoints_mutex_);

        ++reference_counts_by_endpoint_[endpoint];

        std::weak_ptr<impl> weak_self = shared_from_this();

        return std::shared_ptr<void>(nullptr, [weak_self, endpoint](void*) {
            auto strong = weak_self.lock();

            if (!strong)
                return;

            auto& self = *strong;

            std::lock_guard<std::mutex> lock(self.endpoints_mutex_);

            int reference_count_after = --self.reference_counts_by_endpoint_[endpoint];

            if (reference_count_after == 0)
                self.reference_counts_by_endpoint_.erase(endpoint);
        });
    }

  private:
    void propagate(const core::monitor::message& msg)
    {
        std::lock_guard<std::mutex> lock(updates_mutex_);

        try {
            write_osc_event(updates_[msg.path()], msg);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            updates_.erase(msg.path());
        }

        updates_cond_.notify_one();
    }

    template <typename T>
    void do_send(const T& buffers, const std::vector<udp::endpoint>& destinations)
    {
        boost::system::error_code ec;

        for (const auto& endpoint : destinations)
            socket_.send_to(buffers, endpoint, 0, ec);
    }

    void run()
    {
        // http://stackoverflow.com/questions/14993000/the-most-reliable-and-efficient-udp-packet-size
        const int SAFE_DATAGRAM_SIZE = 508;

        try {
            is_running_ = true;

            std::unordered_map<std::string, byte_vector> updates;
            std::vector<udp::endpoint>                   destinations;
            const byte_vector                            bundle_header = write_osc_bundle_start();
            std::vector<byte_vector>                     element_headers;

            while (is_running_) {
                updates.clear();
                destinations.clear();

                {
                    std::unique_lock<std::mutex> cond_lock(updates_mutex_);

                    if (!is_running_)
                        return;

                    if (updates_.empty())
                        updates_cond_.wait(cond_lock);

                    std::swap(updates, updates_);
                }

                {
                    std::lock_guard<std::mutex> lock(endpoints_mutex_);

                    for (const auto& endpoint : reference_counts_by_endpoint_)
                        destinations.push_back(endpoint.first);
                }

                if (destinations.empty())
                    continue;

                std::vector<boost::asio::const_buffers_1> buffers;
                element_headers.resize(std::max(element_headers.size(), updates.size()));

                int  i             = 0;
                auto datagram_size = bundle_header.size();
                buffers.push_back(boost::asio::buffer(bundle_header));

                for (const auto& slot : updates) {
                    write_osc_bundle_element_start(element_headers[i], slot.second);
                    const auto& headers = element_headers;

                    auto size_of_element = headers[i].size() + slot.second.size();

                    if (datagram_size + size_of_element >= SAFE_DATAGRAM_SIZE) {
                        do_send(buffers, destinations);
                        buffers.clear();
                        buffers.push_back(boost::asio::buffer(bundle_header));
                        datagram_size = bundle_header.size();
                    }

                    buffers.push_back(boost::asio::buffer(headers[i]));
                    buffers.push_back(boost::asio::buffer(slot.second));

                    datagram_size += size_of_element;
                    ++i;
                }

                if (!buffers.empty())
                    do_send(buffers, destinations);
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
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

spl::shared_ptr<core::monitor::sink> client::sink() { return impl_; }

}}} // namespace caspar::protocol::osc
