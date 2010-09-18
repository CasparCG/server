#include "../../../StdAfx.h"

#include "tcp_connection.h"

using namespace boost::asio;

namespace caspar { namespace controller { namespace io { namespace tcp {

struct tcp_connection::implementation : public std::enable_shared_from_this<implementation>
{	
	implementation(io_service& io_service, const tcp_connection::read_callback& on_read, const boost::function<void(int)> on_disconnect) 
		: socket_(io_service), on_read_(on_read), on_disconnect_(on_disconnect), tag_(tag_count_++)
	{
	} 
		
	void start_read()
	{
		socket_.async_read_some
		(
			buffer(data_, max_length), 
			[=](const boost::system::error_code& error, size_t bytes_transferred)
			{
				if (!error)
				{
					on_read_(std::wstring(data_, data_ + bytes_transferred), tag_);
					start_read();
				}
				else		
				{
					on_disconnect_(tag_);
					CASPAR_LOG(debug) << error;
				}	
			}
		);
	}

	void start_write(const std::string& message)
	{ 
		bool write_in_progress = !write_queue_.empty();
		write_queue_.push(message);
		if(!write_in_progress)
			write_front();
	}
	
	void write_front()
	{
		auto handle_write = [=](const boost::system::error_code& error, size_t bytes_transferred)
		{
			if(!error)
			{ 
				write_queue_.pop();
				if (!write_queue_.empty())
					write_front();
			}
			else
			{
				on_disconnect_(tag_);
				CASPAR_LOG(debug) << error;
			}
		};

		boost::asio::async_write
		(
			socket_, buffer(write_queue_.front().data(), 
			write_queue_.front().length()),
			handle_write
		);
	}

	int tag() const
	{
		return tag_;
	}
		
	const tcp_connection::read_callback on_read_;

	ip::tcp::socket socket_;

	enum { max_length = 4096 };
	char data_[max_length];

	const int tag_;

	static tbb::atomic<int> tag_count_;
	std::queue<std::string> write_queue_;

	boost::function<void(int)> on_disconnect_;
};

tbb::atomic<int> tcp_connection::implementation::tag_count_;

tcp_connection::tcp_connection(io_service& io_service, const read_callback& on_read, const boost::function<void(int)> on_disconnect) 
	: impl_(new implementation(io_service, on_read, on_disconnect)){}
void tcp_connection::start_read(){impl_->start_read();}
void tcp_connection::start_write(const std::string& message){impl_->start_write(message);}
int tcp_connection::tag() const{return impl_->tag_;}
ip::tcp::socket& tcp_connection::socket(){return impl_->socket_;}

}}}}