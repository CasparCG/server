#pragma once

#include "tcp_connection.h"

#include <boost/asio/io_service.hpp>
#include <boost/signals2.hpp>

#include <memory>
#include <functional>

namespace caspar { namespace controller { namespace io { namespace tcp {

class tcp_server
{
public:
	typedef std::function<void(const std::wstring&, int)> read_callback;

	tcp_server(short port);
	
	boost::signals2::connection subscribe(const read_callback& on_read);
	void start_write(const std::string& message, int tag);

	void run();
	void async_run();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}}