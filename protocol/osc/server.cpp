#include "..\stdafx.h"

#include "server.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <functional>
#include <vector>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

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
	void operator()(const std::wstring& value)			{o << std::string(value.begin(), value.end()).c_str();}
	void operator()(const std::vector<int8_t>& value)	{o << ::osc::Blob(value.data(), static_cast<unsigned long>(value.size()));}
};

std::vector<char> write_osc_event(const monitor::message& e)
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

struct server::impl
{
	boost::asio::io_service						service_;

	udp::endpoint								endpoint_;
	udp::socket									socket_;	

	boost::thread								thread_;

	Concurrency::call<monitor::message>	on_next_;

public:
	impl(udp::endpoint endpoint, 
		 Concurrency::ISource<monitor::message>& source)
		: endpoint_(endpoint)
		, socket_(service_, endpoint_.protocol())
		, thread_(std::bind(&boost::asio::io_service::run, &service_))
		, on_next_([this](const monitor::message& msg){ on_next(msg); })
	{
		source.link_target(&on_next_);
	}

	~impl()
	{		
		thread_.join();
	}

	void on_next(const monitor::message& msg)
	{
		auto data_ptr = spl::make_shared<std::vector<char>>(write_osc_event(msg));
		if(data_ptr->size() >0)
			socket_.async_send_to(boost::asio::buffer(*data_ptr), 
							  endpoint_,
							  boost::bind(&impl::handle_send_to, this, data_ptr,	//data_ptr need to stay alive
							  boost::asio::placeholders::error,
							  boost::asio::placeholders::bytes_transferred));		
	}	

	void handle_send_to(spl::shared_ptr<std::vector<char>> data, const boost::system::error_code& /*error*/, size_t /*bytes_sent*/)
	{
	}
};

server::server(udp::endpoint endpoint, 
			   Concurrency::ISource<monitor::message>& source) 
	: impl_(new impl(endpoint, source))
{
}

server::server(server&& other)
	: impl_(std::move(other.impl_))
{
}

server& server::operator=(server&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

server::~server()
{
}

}}}