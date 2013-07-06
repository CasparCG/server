#pragma once

#include <common/memory/safe_ptr.h>

#include <core/monitor/monitor.h>
#include <boost/asio/ip/udp.hpp>

namespace caspar { namespace protocol { namespace osc {

class client
{
	client(const client&);
	client& operator=(const client&);
public:	

	// Static Members

	// Constructors

	client(boost::asio::ip::udp::endpoint endpoint, 
		   Concurrency::ISource<core::monitor::message>& source);
	
	client(client&&);

	~client();

	// Methods
		
	client& operator=(client&&);
	
	// Properties

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}