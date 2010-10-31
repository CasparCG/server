#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "display_device.h"

#include "../frame/frame_format.h"
#include "../frame/gpu_frame.h"

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/range/algorithm_ext/erase.hpp>

namespace caspar { namespace core { namespace renderer {

class video_sync_clock
{
public:
	video_sync_clock(const frame_format_desc& format_desc)
	{
		period_ = static_cast<long>(get_frame_format_period(format_desc)*1000000.0);
		time_ = boost::posix_time::microsec_clock::local_time();
	}

	void synchronize()
	{
		auto remaining = boost::posix_time::microseconds(period_) - (boost::posix_time::microsec_clock::local_time() - time_);
		if(remaining > boost::posix_time::microseconds(5000))
			boost::this_thread::sleep(remaining - boost::posix_time::microseconds(5000));
		time_ = boost::posix_time::microsec_clock::local_time();
	}
private:
	boost::posix_time::ptime time_;
	long period_;
};

struct display_device::implementation
{
public:
	implementation(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers) : consumers_(consumers), fmt_(format_desc)
	{
		if(consumers.empty())
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("display_device requires atleast one consumer"));

		if(std::any_of(consumers.begin(), consumers.end(), [&](const frame_consumer_ptr& pConsumer){ return pConsumer->get_frame_format_desc() != format_desc;}))
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("All consumers must have same frameformat as display_device."));
		
		needs_clock_ = !std::any_of(consumers.begin(), consumers.end(), std::mem_fn(&frame_consumer::has_sync_clock));
		frame_buffer_.set_capacity(3);
		is_running_ = true;
		display_thread_ = boost::thread([=]{run();});
	}

	~implementation()
	{
		is_running_ = false;
		frame_buffer_.clear();
		display_thread_.join();
	}

	void display(const gpu_frame_ptr& frame)
	{
		if(is_running_)
			frame_buffer_.push(frame);
	}
			
	void run()
	{
		CASPAR_LOG(info) << L"Started display_device thread";
		win32_exception::install_handler();
				
		video_sync_clock clock(fmt_);
				
		while(is_running_)
		{
			if(needs_clock_)
				clock.synchronize();
			
			gpu_frame_ptr frame;
			if(!frame_buffer_.try_pop(frame))
			{
				CASPAR_LOG(trace) << "Display Buffer Underrun";
				frame_buffer_.pop(frame);
			}
			if(frame != nullptr)			
				display_frame(frame);			
		}
		
		CASPAR_LOG(info) << L"Ended display_device thread";
	}

	void display_frame(const gpu_frame_ptr& frame)
	{
		BOOST_FOREACH(const frame_consumer_ptr& consumer, consumers_)
		{
			try
			{
				consumer->prepare(frame); // Could block
				prepared_frames_.push_back(frame);

				if(prepared_frames_.size() > 2)
				{
					consumer->display(prepared_frames_.front()); // Could block
					prepared_frames_.pop_front();
				}
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				boost::range::remove_erase(consumers_, consumer);
				CASPAR_LOG(warning) << "Removed consumer from render-device.";
				if(consumers_.empty())
				{
					CASPAR_LOG(warning) << "No consumers available. Shutting down display-device.";
					is_running_ = false;
				}
			}
		}
	}

	std::deque<gpu_frame_ptr> prepared_frames_;
		
	boost::thread display_thread_;

	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<gpu_frame_ptr> frame_buffer_;

	bool needs_clock_;
	std::vector<frame_consumer_ptr> consumers_;

	frame_format_desc fmt_;
};

display_device::display_device(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers) : impl_(new implementation(format_desc, consumers)){}
void display_device::display(const gpu_frame_ptr& frame){impl_->display(frame);}
}}}