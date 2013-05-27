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

#include "../stdafx.h"

#include "client.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <common/utility/string.h>

#include <functional>
#include <vector>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

using namespace boost::asio::ip;

namespace caspar { namespace protocol { namespace osc {

template<typename T>
struct param_visitor : public boost::static_visitor<void>
{
	T& o;

	param_visitor(T& o)
		: o(o)
	{
	}		
		
	void operator()(const bool value)					{o << value;}
	void operator()(const int32_t value)				{o << static_cast<int64_t>(value);}
	void operator()(const uint32_t value)				{o << static_cast<int64_t>(value);}
	void operator()(const int64_t value)				{o << static_cast<int64_t>(value);}
	void operator()(const uint64_t value)				{o << static_cast<int64_t>(value);}
	void operator()(const float value)					{o << value;}
	void operator()(const double value)					{o << static_cast<float>(value);}
	void operator()(const std::string& value)			{o << value.c_str();}
	void operator()(const std::wstring& value)			{o << narrow(value).c_str();}
	void operator()(const std::vector<int8_t>& value)	{o << ::osc::Blob(value.data(), static_cast<unsigned long>(value.size()));}
};

std::vector<char> write_osc_event(const core::monitor::message& e)
{
	std::array<char, 4096> buffer;
	::osc::OutboundPacketStream o(buffer.data(), static_cast<unsigned long>(buffer.size()));

	o	<< ::osc::BeginMessage(e.path().c_str());
				
	param_visitor<decltype(o)> pd_visitor(o);
	BOOST_FOREACH(auto data, e.data())
		boost::apply_visitor(pd_visitor, data);
				
	o	<< ::osc::EndMessage;
		
	return std::vector<char>(o.Data(), o.Data() + o.Size());
}

struct client::impl
{
	udp::endpoint								endpoint_;
	udp::socket									socket_;	

	Concurrency::call<core::monitor::message>	on_next_;
	
public:
	impl(
			boost::asio::io_service& service,
			udp::endpoint endpoint,
			Concurrency::ISource<core::monitor::message>& source)
		: endpoint_(endpoint)
		, socket_(service, endpoint_.protocol())
		, on_next_([this](const core::monitor::message& msg){ on_next(msg); })
	{
		source.link_target(&on_next_);
	}
	
	void on_next(const core::monitor::message& msg)
	{
		auto data_ptr = make_safe<std::vector<char>>(write_osc_event(msg));

		socket_.async_send_to(boost::asio::buffer(*data_ptr), 
							  endpoint_,
							  boost::bind(&impl::handle_send_to, this,
							  boost::asio::placeholders::error,
							  boost::asio::placeholders::bytes_transferred));		
	}	

	void handle_send_to(const boost::system::error_code& /*error*/, size_t /*bytes_sent*/)
	{
	}
};

client::client(boost::asio::io_service& service, udp::endpoint endpoint, 
			   Concurrency::ISource<core::monitor::message>& source) 
	: impl_(new impl(service, endpoint, source))
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

client::~client()
{
}

}}}
