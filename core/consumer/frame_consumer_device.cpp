#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "frame_consumer_device.h"

#include "../video_format.h"

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/utility/assert.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/circular_buffer.hpp>

namespace caspar { namespace core {
	
struct frame_consumer_device::implementation
{	
	boost::circular_buffer<safe_ptr<const read_frame>> buffer_;

	std::map<int, std::shared_ptr<frame_consumer>> consumers_; // Valid iterators after erase
	
	video_format_desc format_desc_;
	
	executor executor_;	
public:
	implementation( const video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, executor_(L"frame_consumer_device")
	{		
		executor_.set_capacity(2);
		executor_.start();
	}

	~implementation()
	{
		executor_.clear();
		CASPAR_LOG(info) << "Removing consumers from consumer-device.";
		executor_.invoke([this]{consumers_.clear();});
		CASPAR_LOG(info) << "Stopping consumer-device.";
		executor_.stop();
		CASPAR_LOG(info) << "Releasing consumer-device.";
	}

	void add(int index, safe_ptr<frame_consumer>&& consumer)
	{		
		consumer->initialize(format_desc_);
		executor_.invoke([&]
		{
			if(buffer_.capacity() < consumer->buffer_depth())
				buffer_.set_capacity(consumer->buffer_depth());
			consumers_[index] = std::move(consumer);
		});
	}

	void remove(int index)
	{
		executor_.invoke([&]
		{
			auto it = consumers_.find(index);
			if(it != consumers_.end())
			{
				CASPAR_LOG(info) << it->second->print() << L" Removed.";
				consumers_.erase(it);
			}
		});
	}
				
	void send(const safe_ptr<const read_frame>& frame)
	{		
		executor_.begin_invoke([=]
		{	
			buffer_.push_back(std::move(frame));

			if(!buffer_.full())
				return;
	
			auto it = consumers_.begin();
			while(it != consumers_.end())
			{
				try
				{
					it->second->send(buffer_[it->second->buffer_depth()-1]);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					consumers_.erase(it++);
					CASPAR_LOG(warning) << "Removed consumer from frame_consumer_device.";
				}
			}
		});
	}

	std::wstring print() const
	{
		return L"frame_consumer_device";
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			format_desc_ = format_desc;
			buffer_.clear();
			BOOST_FOREACH(auto& consumer, consumers_)
				consumer.second->initialize(format_desc_);
		});
	}
};

frame_consumer_device::frame_consumer_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_consumer_device::add(int index, safe_ptr<frame_consumer>&& consumer){impl_->add(index, std::move(consumer));}
void frame_consumer_device::remove(int index){impl_->remove(index);}
void frame_consumer_device::send(const safe_ptr<const read_frame>& future_frame) { impl_->send(future_frame); }
void frame_consumer_device::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
}}