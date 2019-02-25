/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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
 */

#include "../StdAfx.h"

#include "AsyncEventServer.h"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include <boost/asio.hpp>

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <tbb/mutex.h>

using boost::asio::ip::tcp;

namespace caspar { namespace IO {

class connection;

using connection_set = std::set<spl::shared_ptr<connection>>;

class connection : public spl::enable_shared_from_this<connection>
{
    using lifecycle_map_type = tbb::concurrent_hash_map<std::wstring, std::shared_ptr<void>>;
    using send_queue         = tbb::concurrent_queue<std::string>;

    const spl::shared_ptr<tcp::socket>       socket_;
    std::shared_ptr<boost::asio::io_service> service_;
    const std::wstring                       listen_port_;
    const spl::shared_ptr<connection_set>    connection_set_;
    protocol_strategy_factory<char>::ptr     protocol_factory_;
    std::shared_ptr<protocol_strategy<char>> protocol_;

    std::array<char, 32768> data_;
    lifecycle_map_type      lifecycle_bound_objects_;
    send_queue              send_queue_;
    bool                    is_writing_;

    class connection_holder : public client_connection<char>
    {
        std::weak_ptr<connection> connection_;

      public:
        explicit connection_holder(std::weak_ptr<connection> conn)
            : connection_(std::move(conn))
        {
        }

        void send(std::basic_string<char>&& data, bool skip_log) override
        {
            auto conn = connection_.lock();

            if (conn)
                conn->send(std::move(data));
        }

        void disconnect() override
        {
            auto conn = connection_.lock();

            if (conn)
                conn->disconnect();
        }

        std::wstring address() const override
        {
            auto conn = connection_.lock();

            if (conn)
                return conn->ipv4_address();
            return L"[destroyed-connection]";
        }

        void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) override
        {
            auto conn = connection_.lock();

            if (conn)
                return conn->add_lifecycle_bound_object(key, lifecycle_bound);
        }

        std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) override
        {
            auto conn = connection_.lock();

            if (conn)
                return conn->remove_lifecycle_bound_object(key);
            return std::shared_ptr<void>();
        }
    };

  public:
    static spl::shared_ptr<connection> create(std::shared_ptr<boost::asio::io_service>    service,
                                              spl::shared_ptr<tcp::socket>                socket,
                                              const protocol_strategy_factory<char>::ptr& protocol,
                                              spl::shared_ptr<connection_set>             connection_set)
    {
        spl::shared_ptr<connection> con(
            new connection(std::move(service), std::move(socket), std::move(protocol), std::move(connection_set)));
        con->init();
        con->read_some();
        return con;
    }

    void init() { protocol_ = protocol_factory_->create(spl::make_shared<connection_holder>(shared_from_this())); }

    ~connection() { CASPAR_LOG(debug) << print() << L" connection destroyed."; }

    std::wstring print() const { return L"async_event_server[:" + listen_port_ + L"]"; }

    std::wstring address() const { return u16(socket_->local_endpoint().address().to_string()); }

    std::wstring ipv4_address() const
    {
        return socket_->is_open() ? u16(socket_->remote_endpoint().address().to_string()) : L"no-address";
    }

    void send(std::string&& data)
    {
        send_queue_.push(std::move(data));
        auto self = shared_from_this();
        service_->dispatch([=] { self->do_write(); });
    }

    void disconnect()
    {
        std::weak_ptr<connection> self = shared_from_this();
        service_->dispatch([=] {
            auto strong = self.lock();

            if (strong)
                strong->stop();
        });
    }

    void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound)
    {
        // thread-safe tbb_concurrent_hash_map
        lifecycle_bound_objects_.insert(std::pair<std::wstring, std::shared_ptr<void>>(key, lifecycle_bound));
    }
    std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)
    {
        // thread-safe tbb_concurrent_hash_map
        lifecycle_map_type::const_accessor acc;
        if (lifecycle_bound_objects_.find(acc, key)) {
            auto result = acc->second;
            lifecycle_bound_objects_.erase(acc);
            return result;
        }
        return std::shared_ptr<void>();
    }

  private:
    void do_write() // always called from the asio-service-thread
    {
        if (!is_writing_) {
            std::string data;
            if (send_queue_.try_pop(data)) {
                write_some(std::move(data));
            }
        }
    }

    void stop() // always called from the asio-service-thread
    {
        connection_set_->erase(shared_from_this());

        CASPAR_LOG(info) << print() << L" Client " << ipv4_address() << L" disconnected (" << connection_set_->size()
                         << L" connections).";

        boost::system::error_code ec;
        socket_->shutdown(boost::asio::socket_base::shutdown_type::shutdown_both, ec);
        socket_->close(ec);
    }

    connection(const std::shared_ptr<boost::asio::io_service>& service,
               const spl::shared_ptr<tcp::socket>&             socket,
               const protocol_strategy_factory<char>::ptr&     protocol_factory,
               const spl::shared_ptr<connection_set>&          connection_set)
        : socket_(socket)
        , service_(service)
        , listen_port_(socket_->is_open() ? std::to_wstring(socket_->local_endpoint().port()) : L"no-port")
        , connection_set_(connection_set)
        , protocol_factory_(protocol_factory)
        , is_writing_(false)
    {
        CASPAR_LOG(info) << print() << L" Accepted connection from " << ipv4_address() << L" ("
                         << connection_set_->size() + 1 << L" connections).";
    }

    void handle_read(const boost::system::error_code& error,
                     size_t                           bytes_transferred) // always called from the asio-service-thread
    {
        if (!error) {
            try {
                std::string data(data_.begin(), data_.begin() + bytes_transferred);

                protocol_->parse(data);
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }

            read_some();
        } else if (error != boost::asio::error::operation_aborted)
            stop();
    }

    void handle_write(const spl::shared_ptr<std::string>& str,
                      const boost::system::error_code&    error,
                      size_t bytes_transferred) // always called from the asio-service-thread
    {
        if (!error) {
            if (bytes_transferred != str->size()) {
                str->assign(str->substr(bytes_transferred));
                socket_->async_write_some(boost::asio::buffer(str->data(), str->size()),
                                          std::bind(&connection::handle_write,
                                                    shared_from_this(),
                                                    str,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2));
            } else {
                is_writing_ = false;
                do_write();
            }
        } else if (error != boost::asio::error::operation_aborted && socket_->is_open())
            stop();
    }

    void read_some() // always called from the asio-service-thread
    {
        socket_->async_read_some(
            boost::asio::buffer(data_.data(), data_.size()),
            std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void write_some(std::string&& data) // always called from the asio-service-thread
    {
        is_writing_ = true;
        auto str    = spl::make_shared<std::string>(std::move(data));
        socket_->async_write_some(
            boost::asio::buffer(str->data(), str->size()),
            std::bind(
                &connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
    }

    friend struct AsyncEventServer::implementation;
};

struct AsyncEventServer::implementation : public spl::enable_shared_from_this<implementation>
{
    std::shared_ptr<boost::asio::io_service> service_;
    tcp::acceptor                            acceptor_;
    protocol_strategy_factory<char>::ptr     protocol_factory_;
    spl::shared_ptr<connection_set>          connection_set_;
    std::vector<lifecycle_factory_t>         lifecycle_factories_;
    tbb::mutex                               mutex_;

    implementation(std::shared_ptr<boost::asio::io_service>    service,
                   const protocol_strategy_factory<char>::ptr& protocol,
                   unsigned short                              port)
        : service_(std::move(service))
        , acceptor_(*service_, tcp::endpoint(tcp::v4(), port))
        , protocol_factory_(protocol)
    {
    }

    void stop()
    {
        try {
            acceptor_.cancel();
            acceptor_.close();
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

    ~implementation()
    {
        auto conns_set = connection_set_;

        service_->post([conns_set] {
            auto connections = *conns_set;
            for (auto& connection : connections)
                connection->stop();
        });
    }

    void start_accept()
    {
        spl::shared_ptr<tcp::socket> socket(new tcp::socket(*service_));
        acceptor_.async_accept(
            *socket, std::bind(&implementation::handle_accept, shared_from_this(), socket, std::placeholders::_1));
    }

    void handle_accept(const spl::shared_ptr<tcp::socket>& socket, const boost::system::error_code& error)
    {
        if (!acceptor_.is_open())
            return;

        if (!error) {
            boost::system::error_code ec;
            socket->set_option(boost::asio::socket_base::keep_alive(true), ec);

            if (ec)
                CASPAR_LOG(warning) << print() << L" Failed to enable TCP keep-alive on socket";

            auto conn = connection::create(service_, socket, protocol_factory_, connection_set_);
            connection_set_->insert(conn);

            for (auto& lifecycle_factory : lifecycle_factories_) {
                auto lifecycle_bound = lifecycle_factory(u8(conn->ipv4_address()));
                conn->add_lifecycle_bound_object(lifecycle_bound.first, lifecycle_bound.second);
            }
        }
        start_accept();
    }

    std::wstring print() const
    {
        return L"async_event_server[:" + std::to_wstring(acceptor_.local_endpoint().port()) + L"]";
    }

    void add_client_lifecycle_object_factory(const lifecycle_factory_t& factory)
    {
        auto self = shared_from_this();
        service_->post([=] { self->lifecycle_factories_.push_back(factory); });
    }
};

AsyncEventServer::AsyncEventServer(std::shared_ptr<boost::asio::io_service>    service,
                                   const protocol_strategy_factory<char>::ptr& protocol,
                                   unsigned short                              port)
    : impl_(new implementation(std::move(service), protocol, port))
{
    impl_->start_accept();
}

AsyncEventServer::~AsyncEventServer() { impl_->stop(); }

void AsyncEventServer::add_client_lifecycle_object_factory(const lifecycle_factory_t& factory)
{
    impl_->add_client_lifecycle_object_factory(factory);
}

}} // namespace caspar::IO
