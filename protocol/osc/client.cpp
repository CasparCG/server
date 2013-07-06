#include "..\stdafx.h"

#include "client.h"

#include "oscpack/oscOutboundPacketStream.h"

#include <common/utility/string.h>

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>

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

struct size_visitor : public boost::static_visitor<std::size_t>
{		
	std::size_t operator()(const std::string& value)			{ return value.size() * sizeof(value.front()) * 2; }
	std::size_t operator()(const std::wstring& value)			{ return value.size() * sizeof(value.front()) * 2; }
	std::size_t operator()(const std::vector<int8_t>& value)	{ return value.size() * sizeof(value.front()) * 2; }

	template<typename T>
	std::size_t operator()(const T&)
	{
		return 64;
	}
};

void write_osc_event(byte_vector& destination, const core::monitor::message& e)
{
	try
	{
		std::size_t size = 0;
		
		size_visitor size_visitor;
		BOOST_FOREACH(auto& data, e.data())
			size += boost::apply_visitor(size_visitor, data);

		destination.resize(size);

		::osc::OutboundPacketStream o(reinterpret_cast<char*>(destination.data()), static_cast<unsigned long>(destination.size()));

		o << ::osc::BeginMessage(e.path().c_str());
				
		param_visitor<decltype(o)> param_visitor(o);
		BOOST_FOREACH(auto& data, e.data())
			boost::apply_visitor(param_visitor, data);
				
		o << ::osc::EndMessage;
		
		destination.resize(o.Size());
	}
	catch(...)
	{
	}
}

struct client::impl
{
	boost::asio::io_service													service_;
																			
	udp::endpoint															endpoint_;
	udp::socket																socket_;	
	
	std::array<tbb::concurrent_unordered_map<std::string, byte_vector>, 2>	updates_;
	tbb::atomic<unsigned int>												updates_counter_;		  

	Concurrency::call<core::monitor::message>								call_;
										
	boost::mutex															cond_mutex_;
	boost::condition_variable												cond_;
	tbb::atomic<bool>														is_running_;
	boost::thread															thread_;
	
public:
	impl(udp::endpoint endpoint, 
		 Concurrency::ISource<core::monitor::message>& source)
		: endpoint_(endpoint)
		, socket_(service_, endpoint_.protocol())
		, call_(boost::bind(&impl::on_next, this, _1))
		, thread_(boost::bind(&impl::run, this))
	{
		source.link_target(&call_);		
	}

	~impl()
	{		
		is_running_ = false;

		thread_.join();
	}
	
	void on_next(const core::monitor::message& msg)
	{
		write_osc_event(updates_[updates_counter_ % 1][msg.path()], msg);
		cond_.notify_one();
	}

	void run()
	{
		std::unordered_map<std::string, byte_vector> state;

		is_running_ = true;

		while(is_running_)
		{
			std::vector<boost::asio::mutable_buffers_1>	buffers;

			boost::unique_lock<boost::mutex> cond_lock(cond_mutex_);
					
			if(cond_.timed_wait(cond_lock, boost::posix_time::milliseconds(500)))
			{					
				auto& updates = updates_[(++updates_counter_ - 1) % 1];

				BOOST_FOREACH(auto& slot, updates)		
				{
					if(slot.second.empty())
						continue;

					auto it = state.lower_bound(slot.first);
					if(it == std::end(state) || state.key_comp()(slot.first, it->first))
						it = state.insert(it, std::make_pair(slot.first, slot.second));
					else
						std::swap(it->second, slot.second);

					slot.second.clear();

					buffers.push_back(boost::asio::buffer(it->second));
				}
			}
			else
			{
				BOOST_FOREACH(auto& slot, state)		
				{
					buffers.push_back(boost::asio::buffer(slot.second));
				}
			}		

			if(!buffers.empty())
				socket_.send_to(buffers, endpoint_);
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