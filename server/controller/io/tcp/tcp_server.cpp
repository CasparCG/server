#include "../../../StdAfx.h"

#include "tcp_server.h"

#include "tcp_connection.h"

#include <boost/asio.hpp>

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include <boost/thread.hpp>

using namespace boost::asio;

namespace caspar { namespace controller { namespace io { namespace tcp {

struct tcp_server::implementation
{
	implementation(short port) : acceptor_(io_service_, ip::tcp::endpoint(ip::tcp::v4(), port))
	{
		start_accept();
	}

	~implementation()
	{
		is_running_ = false;
	}

	boost::signals2::connection subscribe(const tcp_server::read_callback& on_read)
	{
		return on_read_.connect(on_read);
	}
	
	void start_write(const std::string& message, int tag)
	{ 
		auto it = connections_.find(tag);
		if(it != connections_.end())
			it->second->start_write(message);
	}

	void start_accept()
	{
		auto new_connection = std::make_shared<tcp_connection>
		(
			acceptor_.io_service(),
			boost::bind(&implementation::on_read, this, _1, _2),
			boost::bind(&implementation::on_disconnect, this, _1)
		);

		acceptor_.async_accept
		(
			new_connection->socket(), 
			[=](const boost::system::error_code& error)
			{
				CASPAR_LOG(info) << common::widen(new_connection->socket().remote_endpoint().address().to_string()) << L" connected.";
				connections_[new_connection->tag()] = new_connection;
				new_connection->start_read();
				start_accept();
			}
		);
	}

	void on_disconnect(int tag)
	{
		auto connection = connections_[tag];
		connections_.erase(tag);
		CASPAR_LOG(info) << common::widen(connection->socket().remote_endpoint().address().to_string()) << L" disconnected. ";
	}

	void on_read(const std::wstring& message, int tag)
	{
		on_read_(message, tag);
	}

	void run()
	{
		is_running_ = true;
		while(is_running_)
		{
			try
			{
				io_service_.run();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(error) << "Restarting tcp_server io_service.";
			}
			if(is_running_)
				boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		}
	}

	boost::asio::io_service io_service_;
	
	boost::signals2::signal<void(const std::wstring&, int)> on_read_;

	ip::tcp::acceptor acceptor_;
	std::map<int, tcp_connection_ptr> connections_;
	tbb::atomic<bool> is_running_;
};

tcp_server::tcp_server(short port) : impl_(new implementation(port)){}
void tcp_server::start_write(const std::string& message, int tag){impl_->start_write(message, tag);}
boost::signals2::connection tcp_server::subscribe(const read_callback& on_read){ return impl_->subscribe(on_read);}
void tcp_server::run() {impl_->run();}
}}}}