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

#include "../stdafx.h"

#include "client.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <common/utility/string.h>

#include <functional>
#include <vector>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <tbb/spin_mutex.h>

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

struct client::impl : public std::enable_shared_from_this<client::impl>
{
	tbb::spin_mutex								endpoints_mutex_;
	std::map<udp::endpoint, int>				reference_counts_by_endpoint_;
	udp::socket									socket_;

	Concurrency::call<core::monitor::message>	on_next_;
	
public:
	impl(
			boost::asio::io_service& service,
			Concurrency::ISource<core::monitor::message>& source)
		: socket_(service, udp::v4())
		, on_next_([this](const core::monitor::message& msg) { on_next(msg); })
	{
		source.link_target(&on_next_);
	}

	std::shared_ptr<void> get_subscription_token(
			const boost::asio::ip::udp::endpoint& endpoint)
	{
		tbb::spin_mutex::scoped_lock lock(endpoints_mutex_);

		++reference_counts_by_endpoint_[endpoint];

		std::weak_ptr<impl> weak_self = shared_from_this();

		return std::shared_ptr<void>(nullptr, [weak_self, endpoint] (void*)
		{
			auto strong = weak_self.lock();

			if (!strong)
				return;

			auto& self = *strong;

			tbb::spin_mutex::scoped_lock lock(self.endpoints_mutex_);

			int reference_count_after =
				--self.reference_counts_by_endpoint_[endpoint];

			if (reference_count_after == 0)
				self.reference_counts_by_endpoint_.erase(endpoint);
		});
	}
	
	void on_next(const core::monitor::message& msg)
	{
		auto data_ptr = make_safe<std::vector<char>>(write_osc_event(msg));

		tbb::spin_mutex::scoped_lock lock(endpoints_mutex_);

		BOOST_FOREACH(auto& elem, reference_counts_by_endpoint_)
		{
			auto& endpoint = elem.first;

			// TODO: We seem to be lucky here, because according to asio
			//       documentation only one async operation can be "in flight"
			//       at any given point in time for a socket. This somehow seems
			//       to work though in the case of UDP and Windows.
			socket_.async_send_to(
					boost::asio::buffer(*data_ptr),
					endpoint,
					boost::bind(
							&impl::handle_send_to,
							this,
							data_ptr, // The data_ptr needs to live
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));		
		}
	}

	void handle_send_to(
			const safe_ptr<std::vector<char>>& /* sent_buffer */,
			const boost::system::error_code& /*error*/,
			size_t /*bytes_sent*/)
	{
	}
};

client::client(
		boost::asio::io_service& service,
		Concurrency::ISource<core::monitor::message>& source) 
	: impl_(new impl(service, source))
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

std::shared_ptr<void> client::get_subscription_token(
			const boost::asio::ip::udp::endpoint& endpoint)
{
	return impl_->get_subscription_token(endpoint);
}

}}}
