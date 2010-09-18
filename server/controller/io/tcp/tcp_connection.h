#pragma once

#include <boost/asio.hpp>

#include <memory>
#include <functional>

namespace caspar { namespace controller { namespace io { namespace tcp {

class tcp_connection
{
public:	
	enum { BUFFER_SIZE = 4096 };

	typedef std::function<void(const std::wstring&, int)> read_callback;

	tcp_connection(boost::asio::io_service& io_service, const read_callback& on_read, const boost::function<void(int)> on_disconnect);
	
	void start_read();
	void start_write(const std::string& message);
	
	int tag() const;
	boost::asio::ip::tcp::socket& socket();

	std::wstring to_string() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<tcp_connection> tcp_connection_ptr;

}}}}