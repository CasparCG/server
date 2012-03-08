/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
// AsyncEventServer.cpp: implementation of the AsyncEventServer class.
//
//////////////////////////////////////////////////////////////////////

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

using boost::asio::ip::tcp;

namespace caspar { namespace IO {
	
bool ConvertWideCharToMultiByte(UINT codePage, const std::wstring& wideString, std::vector<char>& destBuffer)
{
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(codePage, 0, wideString.c_str(), static_cast<int>(wideString.length()), 0, 0, NULL, NULL);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.resize(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(codePage, 0, wideString.c_str(), static_cast<int>(wideString.length()), destBuffer.data(), static_cast<int>(destBuffer.size()), NULL, NULL);
	}
	destBuffer.resize(bytesWritten);
	return (bytesWritten > 0);
}

bool ConvertMultiByteToWideChar(UINT codePage, char* pSource, int sourceLength, std::vector<wchar_t>& wideBuffer, int& countLeftovers)
{
	if(codePage == CP_UTF8) {
		countLeftovers = 0;
		//check from the end of pSource for ev. uncompleted UTF-8 byte sequence
		if(pSource[sourceLength-1] & 0x80) {
			//The last byte is part of a multibyte sequence. If the sequence is not complete, we need to save the partial sequence
			int bytesToCheck = std::min<int>(4, sourceLength);	//a sequence contains a maximum of 4 bytes
			int currentLeftoverIndex = sourceLength-1;
			for(; bytesToCheck > 0; --bytesToCheck, --currentLeftoverIndex) {
				++countLeftovers;
				if(pSource[currentLeftoverIndex] & 0x80) {
					if(pSource[currentLeftoverIndex] & 0x40) { //The two high-bits are set, this is the "header"
						int expectedSequenceLength = 2;
						if(pSource[currentLeftoverIndex] & 0x20)
							++expectedSequenceLength;
						if(pSource[currentLeftoverIndex] & 0x10)
							++expectedSequenceLength;

						if(countLeftovers < expectedSequenceLength) {
							//The sequence is incomplete. Leave the leftovers to be interpreted with the next call
							break;
						}
						//The sequence is complete, there are no leftovers. 
						//...OR...
						//error. Let the conversion-function take the hit.
						countLeftovers = 0;
						break;
					}
				}
				else {
					//error. Let the conversion-function take the hit.
					countLeftovers = 0;
					break;
				}
			}
			if(countLeftovers == 4) {
				//error. Let the conversion-function take the hit.
				countLeftovers = 0;
			}
		}
	}

	int charsWritten = 0;
	int sourceBytesToProcess = sourceLength-countLeftovers;
	int wideBufferCapacity = MultiByteToWideChar(codePage, 0, pSource, sourceBytesToProcess, NULL, NULL);
	if(wideBufferCapacity > 0) 
	{
		wideBuffer.resize(wideBufferCapacity);
		charsWritten = MultiByteToWideChar(codePage, 0, pSource, sourceBytesToProcess, wideBuffer.data(), static_cast<int>(wideBuffer.size()));
	}
	//copy the leftovers to the front of the buffer
	if(countLeftovers > 0) {
		memcpy(pSource, &(pSource[sourceBytesToProcess]), countLeftovers);
	}

	wideBuffer.resize(charsWritten);
	return (charsWritten > 0);
}

class connection;

typedef std::set<spl::shared_ptr<connection>> connection_set;

class connection : public spl::enable_shared_from_this<connection>, public ClientInfo
{    
    spl::shared_ptr<tcp::socket>		socket_; 
	const std::wstring					name_;

	std::array<char, 8192>				data_;

	std::vector<char>					buffer_;

	const spl::shared_ptr<IProtocolStrategy>	protocol_;
	spl::shared_ptr<connection_set>				connection_set_;

public:
    static spl::shared_ptr<connection> create(spl::shared_ptr<tcp::socket> socket, const ProtocolStrategyPtr& protocol, spl::shared_ptr<connection_set> connection_set)
	{
		spl::shared_ptr<connection> con(new connection(socket, protocol, connection_set));
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
		connection_set_->erase(shared_from_this());
	}
	
	/**************/
	
	void stop()
	{
		socket_->shutdown(boost::asio::socket_base::shutdown_both);
		socket_->close();
		CASPAR_LOG(info) << print() << L" Disconnected.";
	}

private:
    connection(const spl::shared_ptr<tcp::socket>& socket, const ProtocolStrategyPtr& protocol, const spl::shared_ptr<connection_set>& connection_set) 
		: socket_(socket)
		, name_((socket_->is_open() ? u16(socket_->local_endpoint().address().to_string() + ":" + boost::lexical_cast<std::string>(socket_->local_endpoint().port())) : L"no-address"))
		, protocol_(protocol)
		, connection_set_(connection_set)
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

				buffer_.insert(buffer_.end(), data_.begin(), data_.begin() + bytes_transferred);
		
				std::vector<wchar_t> str;
				int left_overs;
				ConvertMultiByteToWideChar(protocol_->GetCodepage(), buffer_.data(), static_cast<int>(buffer_.size()), str, left_overs);
				buffer_.resize(left_overs);
		
				protocol_->Parse(str.data(), static_cast<int>(str.size()), shared_from_this());
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			
			read_some();
		}  
		else if (error != boost::asio::error::operation_aborted)
		{
			connection_set_->erase(shared_from_this());
			stop();
		}
		else
			read_some();
    }

    void handle_write(const spl::shared_ptr<std::vector<char>>& data, const boost::system::error_code& error, size_t bytes_transferred)
	{
		if(!error)			
		{
			if(data->size() < 512)
				CASPAR_LOG(trace) << print() << L" Sent: " << u16(std::string(data->begin(), data->end()));
			else
				CASPAR_LOG(trace) << print() << L" Sent more than 512 bytes.";
		}
		else if (error != boost::asio::error::operation_aborted)		
		{
			connection_set_->erase(shared_from_this());
			stop();
		}
    }

	void read_some()
	{
		socket_->async_read_some(boost::asio::buffer(data_.data(), data_.size()), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	
	void write_some(const std::wstring& data)
	{
		spl::shared_ptr<std::vector<char>> str;
		ConvertWideCharToMultiByte(protocol_->GetCodepage(), data, *str);

		socket_->async_write_some(boost::asio::buffer(*str), std::bind(&connection::handle_write, shared_from_this(), str, std::placeholders::_1, std::placeholders::_2));
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
		acceptor_.close();

		service_.post([=]
		{
			BOOST_FOREACH(auto& connection, *connection_set_)
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