#pragma once

#include <common/memory.h>

#include <core/monitor/monitor.h>
#include <boost/asio/ip/udp.hpp>

namespace caspar { namespace protocol { namespace osc {

class server
{
	server(const server&);
	server& operator=(const server&);
public:	

	// Static Members

	// Constructors

	server(boost::asio::ip::udp::endpoint endpoint, 
		   Concurrency::ISource<monitor::message>& source);

	server(server&&);

	~server();

	// Methods

	server& operator=(server&&);

	// Properties

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}