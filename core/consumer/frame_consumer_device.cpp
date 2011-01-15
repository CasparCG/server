#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "frame_consumer_device.h"

#include "../video_format.h"

#include <common/concurrency/executor.h>
#include <common/utility/timer.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/circular_buffer.hpp>

namespace caspar { namespace core {
	
struct frame_consumer_device::implementation
{
	timer clock_;
	executor executor_;	

	boost::circular_buffer<safe_ptr<const read_frame>> buffer_;

	std::list<safe_ptr<frame_consumer>> consumers_; // Valid iterators after erase
	
	const video_format_desc fmt_;

public:
	implementation(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers) 
		: consumers_(consumers.begin(), consumers.end())
		, fmt_(format_desc)
	{		
		if(consumers_.empty())
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("No consumer."));

		std::vector<size_t> depths;
		boost::range::transform(consumers_, std::back_inserter(depths), std::mem_fn(&frame_consumer::buffer_depth));
		buffer_.set_capacity(*boost::range::max_element(depths));
		executor_.set_capacity(3);
		executor_.start();
	}
			
	void consume(safe_ptr<const read_frame>&& frame)
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
					(*it)->send(buffer_[(*it)->buffer_depth()-1]);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					consumers_.erase(it++);
					CASPAR_LOG(warning) << "Removed consumer from frame_consumer_device.";
				}
			}
	
			clock_.wait(fmt_.fps);
		});
	}
};

frame_consumer_device::frame_consumer_device(frame_consumer_device&& other) : impl_(std::move(other.impl_)){}
frame_consumer_device::frame_consumer_device(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers) : impl_(new implementation(format_desc, consumers)){}
void frame_consumer_device::consume(safe_ptr<const read_frame>&& future_frame) { impl_->consume(std::move(future_frame)); }
}}