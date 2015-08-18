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
#include <string>
#include <set>
#include <memory>
#include <functional>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <tbb/mutex.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>

using boost::asio::ip::tcp;

namespace caspar { namespace IO {
	
class connection;

typedef std::set<spl::shared_ptr<connection>> connection_set;

class connection : public spl::enable_shared_from_this<connection>
{   
	typedef tbb::concurrent_hash_map<std::wstring, std::shared_ptr<void>> lifecycle_map_type;
	typedef tbb::concurrent_queue<std::string>	send_queue;

    const spl::shared_ptr<tcp::socket>				socket_; 
	std::shared_ptr<boost::asio::io_service>		service_;
	const spl::shared_ptr<connection_set>			connection_set_;
	const std::wstring								name_;
	protocol_strategy_factory<char>::ptr			protocol_factory_;
	std::shared_ptr<protocol_strategy<char>>		protocol_;

	std::array<char, 32768>							data_;
	lifecycle_map_type								lifecycle_bound_objects_;
	send_queue										send_queue_;
	bool											is_writing_;

	class connection_holder : public client_connection<char>
	{
		std::weak_ptr<connection> connection_;
	public:
		explicit connection_holder(std::weak_ptr<connection> conn) : connection_(std::move(conn))
		{}

		void send(std::basic_string<char>&& data) override
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

		std::wstring print() const override
		{
			auto conn = connection_.lock();

			if (conn)
				return conn->print();
			else
				return L"[destroyed-connection]";
		}

		std::wstring address() const override
		{
			auto conn = connection_.lock();

			if (conn)
				return conn->address();
			else
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
			else
				return std::shared_ptr<void>();
		}
	};

public:
	static spl::shared_ptr<connection> create(std::shared_ptr<boost::asio::io_service> service, spl::shared_ptr<tcp::socket> socket, const protocol_strategy_factory<char>::ptr& protocol, spl::shared_ptr<connection_set> connection_set)
	{
		spl::shared_ptr<connection> con(new connection(std::move(service), std::move(socket), std::move(protocol), std::move(connection_set)));
		con->read_some();
		return con;
    }

	~connection()
	{
		CASPAR_LOG(info) << print() << L" connection destroyed.";
	}

	std::wstring print() const
	{
		return L"[" + name_ + L"]";
	}

	std::wstring address() const
	{
		return u16(socket_->local_endpoint().address().to_string());
	}
	
	virtual void send(std::string&& data)
	{
		send_queue_.push(std::move(data));
		service_->dispatch([=] { do_write(); });
	}

	virtual void disconnect()
	{
		service_->dispatch([=] { stop(); });
	}

	void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound)
	{
		//thread-safe tbb_concurrent_hash_map
		lifecycle_bound_objects_.insert(std::pair<std::wstring, std::shared_ptr<void>>(key, lifecycle_bound));
	}
	std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)
	{
		//thread-safe tbb_concurrent_hash_map
		lifecycle_map_type::const_accessor acc;
		if(lifecycle_bound_objects_.find(acc, key))
		{
			auto result = acc->second;
			lifecycle_bound_objects_.erase(acc);
			return result;
		}
		return std::shared_ptr<void>();
	}

	/**************/
private:
	void do_write()	//always called from the asio-service-thread
	{
		if(!is_writing_)
		{
			std::string data;
			if(send_queue_.try_pop(data))
			{
				write_some(std::move(data));
			}
		}
	}

	void stop()	//always called from the asio-service-thread
	{
		connection_set_->erase(shared_from_this());
		try
		{
			socket_->close();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		CASPAR_LOG(info) << print() << L" Disconnected.";
	}

	const std::string ipv4_address() const
	{
		return socket_->is_open() ? socket_->remote_endpoint().address().to_string() : "no-address";
	}

    connection(const std::shared_ptr<boost::asio::io_service>& service, const spl::shared_ptr<tcp::socket>& socket, const protocol_strategy_factory<char>::ptr& protocol_factory, const spl::shared_ptr<connection_set>& connection_set) 
		: socket_(socket)
		, service_(service)
		, name_((socket_->is_open() ? u16(socket_->local_endpoint().address().to_string() + ":" + boost::lexical_cast<std::string>(socket_->local_endpoint().port())) : L"no-address"))
		, connection_set_(connection_set)
		, protocol_factory_(protocol_factory)
		, is_writing_(false)
	{
		CASPAR_LOG(info) << print() << L" Connected.";
    }

	protocol_strategy<char>& protocol()	//always called from the asio-service-thread
	{
		if (!protocol_)
			protocol_ = protocol_factory_->create(spl::make_shared<connection_holder>(shared_from_this()));

		return *protocol_;
	}
			
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) 	//always called from the asio-service-thread
	{		
		if(!error)
		{
			try
			{
				std::string data(data_.begin(), data_.begin() + bytes_transferred);

				CASPAR_LOG(trace) << print() << L" Received: " << u16(data);

				protocol().parse(data);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			
			read_some();
		}  
		else if (error != boost::asio::error::operation_aborted)
			stop();		
    }

    void handle_write(const spl::shared_ptr<std::string>& str, const boost::system::error_code& error, size_t bytes_transferred)	//always called from the asio-service-thread
	{
		if(!error)
		{
			CASPAR_LOG(trace) << print() << L" Sent: " << (str->size() < 512 ? u16(*str) : L"more than 512 bytes.");
			if(bytes_transferred != str->size())
			{
				str->assign(str->substr(bytes_transferred));
				socket_->async_write_some(boost::asio::buffer(str->data(), str->size()), std::bind(&connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
			}
			else
			{
				is_writing_ = false;
				do_write();
			}
		}
		else if (error != boost::asio::error::operation_aborted)		
			stop();
    }

	void read_some()	//always called from the asio-service-thread
	{
		socket_->async_read_some(boost::asio::buffer(data_.data(), data_.size()), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	
	void write_some(std::string&& data)	//always called from the asio-service-thread
	{
		is_writing_ = true;
		auto str = spl::make_shared<std::string>(std::move(data));
		socket_->async_write_some(boost::asio::buffer(str->data(), str->size()), std::bind(&connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
	}

	friend struct AsyncEventServer::implementation;
};

struct AsyncEventServer::implementation
{
	std::shared_ptr<boost::asio::io_service>	service_;
	tcp::acceptor								acceptor_;
	protocol_strategy_factory<char>::ptr		protocol_factory_;
	spl::shared_ptr<connection_set>				connection_set_;
	std::vector<lifecycle_factory_t>			lifecycle_factories_;
	tbb::mutex									mutex_;

	implementation(std::shared_ptr<boost::asio::io_service> service, const protocol_strategy_factory<char>::ptr& protocol, unsigned short port)
		: service_(std::move(service))
		, acceptor_(*service_, tcp::endpoint(tcp::v4(), port))
		, protocol_factory_(protocol)
	{
		start_accept();
	}

	~implementation()
	{
		try
		{
			acceptor_.cancel();
			acceptor_.close();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		auto conns_set = connection_set_;

		service_->post([conns_set]
		{
			auto connections = *conns_set;
			for (auto& connection : connections)
				connection->stop();
		});
	}
		
	void start_accept() 
	{
		spl::shared_ptr<tcp::socket> socket(new tcp::socket(*service_));
		acceptor_.async_accept(*socket, std::bind(&implementation::handle_accept, this, socket, std::placeholders::_1));
    }

	void handle_accept(const spl::shared_ptr<tcp::socket>& socket, const boost::system::error_code& error) 
	{
		if (!acceptor_.is_open())
			return;
		
        if (!error)
		{
			auto conn = connection::create(service_, socket, protocol_factory_, connection_set_);
			connection_set_->insert(conn);

			for (auto& lifecycle_factory : lifecycle_factories_)
			{
				auto lifecycle_bound = lifecycle_factory(conn->ipv4_address());
				conn->add_lifecycle_bound_object(lifecycle_bound.first, lifecycle_bound.second);
			}
		}
		start_accept();
    }

	void add_client_lifecycle_object_factory(const lifecycle_factory_t& factory)
	{
		service_->post([=]{ lifecycle_factories_.push_back(factory); });
	}
};

AsyncEventServer::AsyncEventServer(
		std::shared_ptr<boost::asio::io_service> service, const protocol_strategy_factory<char>::ptr& protocol, unsigned short port)
	: impl_(new implementation(std::move(service), protocol, port)) {}

AsyncEventServer::~AsyncEventServer() {}
void AsyncEventServer::add_client_lifecycle_object_factory(const lifecycle_factory_t& factory) { impl_->add_client_lifecycle_object_factory(factory); }

}}
