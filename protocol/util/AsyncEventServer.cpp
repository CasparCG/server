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

#include "ProtocolStrategy.h"

#include <algorithm>
#include <array>
#include <string>
#include <set>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/algorithm/string/split.hpp>

using boost::asio::ip::tcp;

namespace caspar { namespace IO {
	
class connection;

typedef std::set<spl::shared_ptr<connection>> connection_set;

class connection : public spl::enable_shared_from_this<connection>, public ClientInfo
{    
    const spl::shared_ptr<tcp::socket>			socket_; 
	const spl::shared_ptr<connection_set>		connection_set_;
	const std::wstring							name_;
	const spl::shared_ptr<IProtocolStrategy>	protocol_;

	std::array<char, 32768>						data_;
	std::string									input_;

public:
    static spl::shared_ptr<connection> create(spl::shared_ptr<tcp::socket> socket, const ProtocolStrategyPtr& protocol, spl::shared_ptr<connection_set> connection_set)
	{
		spl::shared_ptr<connection> con(new connection(std::move(socket), std::move(protocol), std::move(connection_set)));
		con->read_some();
		return con;
    }

	std::wstring print() const
	{
		return L"[" + name_ + L"]";
	}
	
	/* ClientInfo */
	
	virtual void Send(const std::wstring& data)
	{
		write_some(data);
	}

	virtual void Disconnect()
	{
		stop();
	}
	
	/**************/
	
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

private:
    connection(const spl::shared_ptr<tcp::socket>& socket, const ProtocolStrategyPtr& protocol, const spl::shared_ptr<connection_set>& connection_set) 
		: socket_(socket)
		, name_((socket_->is_open() ? u16(socket_->local_endpoint().address().to_string() + ":" + boost::lexical_cast<std::string>(socket_->local_endpoint().port())) : L"no-address"))
		, connection_set_(connection_set)
		, protocol_(protocol)
	{
		CASPAR_LOG(info) << print() << L" Connected.";
    }
			
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) 
	{		
		if(!error)
		{
			try
			{
				CASPAR_LOG(trace) << print() << L" Received: " << u16(std::string(data_.begin(), data_.begin() + bytes_transferred));

				input_.append(data_.begin(), data_.begin() + bytes_transferred);
				
				std::vector<std::string> split;
				boost::iter_split(split, input_, boost::algorithm::first_finder("\r\n"));
				
				input_ = std::move(split.back());
				split.pop_back();

				BOOST_FOREACH(auto cmd, split)
				{
					auto u16cmd = boost::locale::conv::to_utf<wchar_t>(cmd, protocol_->GetCodepage()) + L"\r\n";
					protocol_->Parse(u16cmd.data(), static_cast<int>(u16cmd.size()), shared_from_this());
				}
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
			CASPAR_LOG(trace) << print() << L" Sent: " << (data->size() < 512 ? u16(std::string(data->begin(), data->end())) : L"more than 512 bytes.");		
		else if (error != boost::asio::error::operation_aborted)		
			stop();		
    }

	void read_some()
	{
		socket_->async_read_some(boost::asio::buffer(data_.data(), data_.size()), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	
	void write_some(const std::wstring& data)
	{
		auto str = spl::make_shared<std::string>(boost::locale::conv::from_utf<wchar_t>(data, protocol_->GetCodepage()));
		socket_->async_write_some(boost::asio::buffer(str->data(), str->size()), std::bind(&connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
	}
};

struct AsyncEventServer::implementation
{
	boost::asio::io_service					service_;
	tcp::acceptor							acceptor_;
	spl::shared_ptr<IProtocolStrategy>		protocol_;
	spl::shared_ptr<connection_set>			connection_set_;
	boost::thread							thread_;

	implementation(const spl::shared_ptr<IProtocolStrategy>& protocol, unsigned short port)
		: acceptor_(service_, tcp::endpoint(tcp::v4(), port))
		, protocol_(protocol)
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
			connection_set_->insert(connection::create(socket, protocol_, connection_set_));

		start_accept();
    }
};

AsyncEventServer::AsyncEventServer(const spl::shared_ptr<IProtocolStrategy>& protocol, unsigned short port) : impl_(new implementation(protocol, port)){}
AsyncEventServer::~AsyncEventServer(){}
}}