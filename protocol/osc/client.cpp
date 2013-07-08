#include "..\stdafx.h"

#include "client.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <common/utility/string.h>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/cache_aligned_allocator.h>

#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace boost::asio::ip;

namespace caspar { namespace protocol { namespace osc {
	
template<typename T>
struct no_init_proxy
{
    T value;

    no_init_proxy() 
	{
		static_assert(sizeof(no_init_proxy) == sizeof(T), "invalid size");
        static_assert(__alignof(no_init_proxy) == __alignof(T), "invalid alignment");
    }
};

typedef std::vector<no_init_proxy<char>, tbb::cache_aligned_allocator<no_init_proxy<char>>> byte_vector;

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


void write_osc_event(byte_vector& destination, const core::monitor::message& e)
{		
	destination.resize(4096);

	::osc::OutboundPacketStream o(reinterpret_cast<char*>(destination.data()), static_cast<unsigned long>(destination.size()));

	o << ::osc::BeginMessage(e.path().c_str());
				
	param_visitor<decltype(o)> param_visitor(o);
	BOOST_FOREACH(const auto& data, e.data())
		boost::apply_visitor(param_visitor, data);
				
	o << ::osc::EndMessage;
		
	destination.resize(o.Size());
}

struct client::impl
{
	boost::asio::io_service							service_;
													
	udp::endpoint									endpoint_;
	udp::socket										socket_;	
	
	std::unordered_map<std::string, byte_vector>	updates_;
	boost::mutex									updates_mutex_;								
	boost::condition_variable						updates_cond_;

	tbb::atomic<bool>								is_running_;

	boost::thread									thread_;

	Concurrency::call<core::monitor::message>		on_next_;
	
public:
	impl(udp::endpoint endpoint, 
		 Concurrency::ISource<core::monitor::message>& source)
		: endpoint_(endpoint)
		, socket_(service_, endpoint_.protocol())
		, thread_(boost::bind(&impl::run, this))
		, on_next_(boost::bind(&impl::on_next, this, _1))
	{
		source.link_target(&on_next_);	
	}

	~impl()
	{		
		is_running_ = false;

		updates_cond_.notify_one();

		thread_.join();
	}
	
	void on_next(const core::monitor::message& msg)
	{		
		boost::lock_guard<boost::mutex> lock(updates_mutex_);

		try
		{
			write_osc_event(updates_[msg.path()], msg);
		}
		catch(...)
		{
			updates_.erase(msg.path());
		}

		updates_cond_.notify_one();
	}

	void run()
	{
		try
		{
			is_running_ = true;

			std::unordered_map<std::string, byte_vector> updates;
			

			while(is_running_)
			{		
				{			
					boost::unique_lock<boost::mutex> cond_lock(updates_mutex_);

					updates_cond_.wait(cond_lock);		

					std::swap(updates, updates_);
				}

				std::vector<boost::asio::const_buffers_1> buffers;

				BOOST_FOREACH(const auto& slot, updates)		
					buffers.push_back(boost::asio::buffer(slot.second));
			
				boost::system::error_code ec;
				socket_.send_to(buffers, endpoint_, 0, ec);
			
				updates.clear();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
};

client::client(udp::endpoint endpoint, 
			   Concurrency::ISource<core::monitor::message>& source) 
	: impl_(new impl(endpoint, source))
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