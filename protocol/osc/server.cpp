#include "..\stdafx.h"

#include "server.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <algorithm>
#include <array>
#include <string>
#include <set>
#include <regex>
#include <vector>

#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread/mutex.hpp>

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
	void operator()(const int32_t value)				{o << value;}
	void operator()(const uint32_t value)				{o << value;}
	void operator()(const int64_t value)				{o << value;}
	void operator()(const uint64_t value)				{o << value;}
	void operator()(const float value)					{o << value;}
	void operator()(const double value)					{o << static_cast<float>(value);}
	void operator()(const std::string& value)			{o << value.c_str();}
	void operator()(const std::vector<int8_t>& value)	{o << ::osc::Blob(value.data(), static_cast<unsigned long>(value.size()));}
	void operator()(const monitor::duration& value)
	{
		o << boost::chrono::duration_cast<boost::chrono::duration<double, boost::ratio<1, 1>>>(value).count();
	}
};

std::vector<char> write_osc_event(const monitor::event& e)
{
	std::array<char, 4096> buffer;
	::osc::OutboundPacketStream o(buffer.data(), static_cast<unsigned long>(buffer.size()));

	o	<< ::osc::BeginMessage(e.path().str().c_str());
				
	param_visitor<decltype(o)> pd_visitor(o);
	BOOST_FOREACH(auto param, e.params())
		boost::apply_visitor(pd_visitor, param);
				
	o	<< ::osc::EndMessage;
		
	return std::vector<char>(o.Data(), o.Data() + o.Size());
}

class connection;

typedef std::set<spl::shared_ptr<connection>> connection_set;

class connection : public spl::enable_shared_from_this<connection>
{    
    spl::shared_ptr<tcp::socket>			socket_; 
	boost::optional<std::regex>		regex_;

	std::array<char, 8192>			data_;
	
	spl::shared_ptr<connection_set>		connection_set_;

	std::string						input_;

public:
    static spl::shared_ptr<connection> create(spl::shared_ptr<tcp::socket> socket, spl::shared_ptr<connection_set> connection_set)
	{
		auto con = spl::shared_ptr<connection>(new connection(std::move(socket), std::move(connection_set)));
		con->read_some();
		return con;
    }
	
	void stop()
	{
		socket_->shutdown(boost::asio::socket_base::shutdown_both);
		socket_->close();
	}
		
	std::wstring print() const
	{
		return L"[" + (socket_->is_open() ? u16(socket_->remote_endpoint().address().to_string()) : L"n/a") + L"]";
	}
		
	void on_next(const monitor::event& e)
	{	
		if(regex_ && std::regex_search(e.path().str(), *regex_))
			return;	

		auto data_ptr = spl::make_shared<std::vector<char>>(write_osc_event(e));
		auto size = data_ptr->size();
		char* size_ptr = reinterpret_cast<char*>(&size);

		data_ptr->insert(std::begin(*data_ptr), size_ptr, size_ptr + sizeof(int32_t));
		socket_->async_write_some(boost::asio::buffer(*data_ptr), std::bind(&connection::handle_write, shared_from_this(), data_ptr, std::placeholders::_1, std::placeholders::_2));	
	}
	
private:
    connection(spl::shared_ptr<tcp::socket> socket, spl::shared_ptr<connection_set> connection_set) 
		: socket_(std::move(socket))
		, connection_set_(std::move(connection_set))
	{
    }
					
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) 
	{		
		if(!error)
		{
			try
			{
				on_read(std::string(data_.begin(), data_.begin() + bytes_transferred));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			
			read_some();
		}  
		else if (error != boost::asio::error::operation_aborted)
			connection_set_->erase(shared_from_this());
		else
			read_some();
    }

    void handle_write(const spl::shared_ptr<std::vector<char>>& data, const boost::system::error_code& error, size_t bytes_transferred)
	{
		if(!error)			
		{
		}
		else if (error != boost::asio::error::operation_aborted)
			connection_set_->erase(shared_from_this());
    }

	void read_some()
	{
		socket_->async_read_some(boost::asio::buffer(data_.data(), data_.size()), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
		
	void on_read(std::string str)
	{
		input_ += str;

		std::vector<std::string> commands;
		boost::iter_split(commands, input_, boost::algorithm::first_finder("\r\n"));
		
		if(commands.size() == 1)
			return;

		input_ = commands.back();
		commands.pop_back();	

		if(commands.back() == ".*")
			regex_.reset();
		else
			regex_ = std::regex(commands.back());
	}
};

class tcp_observer : public reactive::observer<monitor::event>
{
	boost::asio::io_service			service_;
	tcp::acceptor					acceptor_;
	spl::shared_ptr<connection_set>	connection_set_;
	boost::thread					thread_;
	
public:
	tcp_observer(unsigned short port)
		: acceptor_(service_, tcp::endpoint(tcp::v4(), port))
		, thread_(std::bind(&boost::asio::io_service::run, &service_))
	{
		start_accept();	
	}

	~tcp_observer()
	{		
		acceptor_.close();
		BOOST_FOREACH(auto& connection, *connection_set_)
			connection->stop();

		thread_.join();
	}
	
	void on_next(const monitor::event& e) override
	{
		service_.post([=]
		{
			BOOST_FOREACH(auto& connection, *connection_set_)
				connection->on_next(e);
		});
	}	
private:		
    void start_accept() 
	{
		auto socket = spl::make_shared<tcp::socket>(service_);
		acceptor_.async_accept(*socket, std::bind(&tcp_observer::handle_accept, this, socket, std::placeholders::_1));
    }

	void handle_accept(const spl::shared_ptr<tcp::socket>& socket, const boost::system::error_code& error) 
	{
		if (!acceptor_.is_open())
			return;

        if (!error)		
			connection_set_->insert(connection::create(socket, connection_set_));
        
		start_accept();
    }
};

server::server(unsigned short port) 
	: impl_(new tcp_observer(port)){}
void server::on_next(const monitor::event& e){impl_->on_next(e);}

}}}