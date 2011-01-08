#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "frame_consumer_device.h"

#include "../format/video_format.h"

#include <common/concurrency/executor.h>
#include <common/utility/timer.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
	
struct frame_consumer_device::implementation
{
public:
	implementation(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers) : consumers_(consumers), fmt_(format_desc)
	{		
		std::vector<size_t> depths;
		boost::range::transform(consumers_, std::back_inserter(depths), std::mem_fn(&frame_consumer::buffer_depth));
		max_depth_ = *boost::range::max_element(depths);
		executor_.start();
	}
					
	void tick(const safe_ptr<const read_frame>& frame)
	{
		buffer_.push_back(frame);
	
		boost::range::for_each(consumers_, [&](const safe_ptr<frame_consumer>& consumer)
		{
			size_t offset = max_depth_ - consumer->buffer_depth();
			if(offset < buffer_.size())
				consumer->send(*(buffer_.begin() + offset));
		});
			
		frame_consumer::sync_mode sync = frame_consumer::ready;
		boost::range::for_each(consumers_, [&](const safe_ptr<frame_consumer>& consumer)
		{
			try
			{
				size_t offset = max_depth_ - consumer->buffer_depth();
				if(offset >= buffer_.size())
					return;

				if(consumer->synchronize() == frame_consumer::clock)
					sync = frame_consumer::clock;
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				boost::range::remove_erase(consumers_, consumer);
				CASPAR_LOG(warning) << "Removed consumer from frame_consumer_device.";
			}
		});
	
		if(sync != frame_consumer::clock)
			clock_.wait();

		if(buffer_.size() >= max_depth_)
			buffer_.pop_front();
	}

	void consume(safe_ptr<const read_frame>&& frame)
	{		
		if(executor_.size() < 3)
			executor_.begin_invoke([=]{tick(frame);});
		else
			executor_.invoke([=]{tick(frame);});
	}

	timer clock_;
	executor executor_;	

	size_t max_depth_;
	std::deque<safe_ptr<const read_frame>> buffer_;		

	std::vector<safe_ptr<frame_consumer>> consumers_;
	
	const video_format_desc& fmt_;
};

frame_consumer_device::frame_consumer_device(frame_consumer_device&& other) : impl_(std::move(other.impl_)){}
frame_consumer_device::frame_consumer_device(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers) : impl_(new implementation(format_desc, consumers)){}
void frame_consumer_device::consume(safe_ptr<const read_frame>&& future_frame) { impl_->consume(std::move(future_frame)); }
}}