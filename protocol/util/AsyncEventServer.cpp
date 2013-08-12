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

#include "..\stdafx.h"

#include "AsyncEventServer.h"

#include <algorithm>
#include <array>
#include <string>
#include <set>
#include <memory>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#include <tbb\mutex.h>

using boost::asio::ip::tcp;

namespace caspar { namespace IO {
	
class connection;

typedef std::set<spl::shared_ptr<connection>> connection_set;

class connection : public spl::enable_shared_from_this<connection>
{   
    const spl::shared_ptr<tcp::socket>				socket_; 
	boost::asio::io_service&						service_;
	const spl::shared_ptr<connection_set>			connection_set_;
	const std::wstring								name_;
	protocol_strategy_factory<char>::ptr			protocol_factory_;
	std::shared_ptr<protocol_strategy<char>>		protocol_;

	std::array<char, 32768>							data_;
	std::map<std::wstring, std::shared_ptr<void>>	lifecycle_bound_objects_;

	class connection_holder : public client_connection<char>
	{
		std::weak_ptr<connection> connection_;
	public:
		explicit connection_holder(std::weak_ptr<connection> conn) : connection_(conn)
		{}

		virtual void send(std::basic_string<char>&& data)
		{
			//TODO: need to implement a send-queue
			auto conn = connection_.lock();
			conn->send(std::move(data));
		}
		virtual void disconnect()
		{
			auto conn = connection_.lock();
			conn->disconnect();
		}
		virtual std::wstring print() const
		{
			auto conn = connection_.lock();
			return conn->print();
		}

		virtual void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound)
		{
			auto conn = connection_.lock();
			return conn->add_lifecycle_bound_object(key, lifecycle_bound);
		}
		virtual std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)
		{
			auto conn = connection_.lock();
			return conn->remove_lifecycle_bound_object(key);
		}
	};

public:
    static spl::shared_ptr<connection> create(spl::shared_ptr<tcp::socket> socket, const protocol_strategy_factory<char>::ptr& protocol, spl::shared_ptr<connection_set> connection_set)
	{
		spl::shared_ptr<connection> con(new connection(std::move(socket), std::move(protocol), std::move(connection_set)));
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
	
	virtual void send(std::string&& data)
	{
		write_some(std::move(data));
	}

	virtual void disconnect()
	{
		service_.dispatch([=] { stop(); });
	}

	void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void> lifecycle_bound)
	{
		//TODO: needs protection from evil concurrent access
		//tbb::concurrent_hash_map ?
		lifecycle_bound_objects_.insert(std::pair<std::wstring, std::shared_ptr<void>>(key, lifecycle_bound));
	}
	std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)
	{
		//TODO: needs protection from evil concurrent access
		//tbb::concurrent_hash_map ?
		auto it = lifecycle_bound_objects_.find(key);
		if(it != lifecycle_bound_objects_.end())
		{
			auto result = (*it).second;
			lifecycle_bound_objects_.erase(it);
			return result;
		}
		return std::shared_ptr<void>();
	}

	/**************/
private:
	void stop()
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
		return socket_->is_open() ? socket_->local_endpoint().address().to_string() : "no-address";
	}

    connection(const spl::shared_ptr<tcp::socket>& socket, const protocol_strategy_factory<char>::ptr& protocol_factory, const spl::shared_ptr<connection_set>& connection_set) 
		: socket_(socket)
		, service_(socket->get_io_service())
		, name_((socket_->is_open() ? u16(socket_->local_endpoint().address().to_string() + ":" + boost::lexical_cast<std::string>(socket_->local_endpoint().port())) : L"no-address"))
		, connection_set_(connection_set)
		, protocol_factory_(protocol_factory)
	{
		CASPAR_LOG(info) << print() << L" Connected.";
    }

	protocol_strategy<char>& protocol()
	{
		if (!protocol_)
			protocol_ = protocol_factory_->create(spl::make_shared<connection_holder>(shared_from_this()));

		return *protocol_;
	}
			
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) 
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

    void handle_write(const spl::shared_ptr<std::string>& data, const boost::system::error_code& error, size_t bytes_transferred)
	{
		if(!error)			
			CASPAR_LOG(trace) << print() << L" Sent: " << (data->size() < 512 ? u16(*data) : L"more than 512 bytes.");		
		else if (error != boost::asio::error::operation_aborted)		
			stop();
    }

	void read_some()
	{
		socket_->async_read_some(boost::asio::buffer(data_.data(), data_.size()), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	
	void write_some(std::string&& data)
	{
		auto str = spl::make_shared<std::string>(std::move(data));
		socket_->async_write_some(boost::asio::buffer(str->data(), str->size()), std::bind(&connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
	}

	friend struct AsyncEventServer::implementation;
};

struct AsyncEventServer::implementation
{
	boost::asio::io_service					service_;
	tcp::acceptor							acceptor_;
	protocol_strategy_factory<char>::ptr	protocol_factory_;
	spl::shared_ptr<connection_set>			connection_set_;
	boost::thread							thread_;
	std::vector<lifecycle_factory_t>		lifecycle_factories_;
	tbb::mutex mutex_;

	implementation(const protocol_strategy_factory<char>::ptr& protocol, unsigned short port)
		: acceptor_(service_, tcp::endpoint(tcp::v4(), port))
		, protocol_factory_(protocol)
		, thread_(std::bind(&boost::asio::io_service::run, &service_))
	{
		start_accept();
	}

	~implementation()
	{
		try
		{
			acceptor_.close();			
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		service_.post([=]
		{
			auto connections = *connection_set_;
			BOOST_FOREACH(auto& connection, connections)
				connection->stop();				
		});

		thread_.join();
	}
		
    void start_accept() 
	{
		spl::shared_ptr<tcp::socket> socket(new tcp::socket(service_));
		acceptor_.async_accept(*socket, std::bind(&implementation::handle_accept, this, socket, std::placeholders::_1));
    }

	void handle_accept(const spl::shared_ptr<tcp::socket>& socket, const boost::system::error_code& error) 
	{
		if (!acceptor_.is_open())
			return;
		
        if (!error)
		{
			auto conn = connection::create(socket, protocol_factory_, connection_set_);
			connection_set_->insert(conn);

			BOOST_FOREACH(auto& lifecycle_factory, lifecycle_factories_)
			{
				auto lifecycle_bound = lifecycle_factory(conn->ipv4_address());
				conn->add_lifecycle_bound_object(lifecycle_bound.first, lifecycle_bound.second);
			}
		}
		start_accept();
    }

	void add_client_lifecycle_object_factory(const lifecycle_factory_t& factory)
	{
		service_.post([=]{ lifecycle_factories_.push_back(factory); });
	}
};

AsyncEventServer::AsyncEventServer(
		const protocol_strategy_factory<char>::ptr& protocol, unsigned short port)
	: impl_(new implementation(protocol, port)) {}

AsyncEventServer::~AsyncEventServer() {}
void AsyncEventServer::add_client_lifecycle_object_factory(const lifecycle_factory_t& factory) { impl_->add_client_lifecycle_object_factory(factory); }

}}