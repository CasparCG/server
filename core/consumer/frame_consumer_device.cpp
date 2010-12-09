#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "frame_consumer_device.h"

#include "../format/video_format.h"
#include "../processor/write_frame.h"
#include "../processor/frame_processor_device.h"

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/range/algorithm_ext/erase.hpp>

namespace caspar { namespace core {

class video_sync_clock
{
public:
	video_sync_clock(const video_format_desc& format_desc)
	{
		period_ = static_cast<long>(format_desc.period*1000000.0);
		time_ = boost::posix_time::microsec_clock::local_time();
	}

	void synchronize()
	{
		auto remaining = boost::posix_time::microseconds(period_) - 	(boost::posix_time::microsec_clock::local_time() - time_);
		if(remaining > boost::posix_time::microseconds(5000))
			boost::this_thread::sleep(remaining - boost::posix_time::microseconds(5000));
		time_ = boost::posix_time::microsec_clock::local_time();
	}
private:
	boost::posix_time::ptime time_;
	long period_;
};

struct frame_consumer_device::implementation
{
public:
	implementation(const frame_processor_device_ptr& frame_processor, const video_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers) 
		: frame_processor_(frame_processor), consumers_(consumers), fmt_(format_desc)
	{
		if(consumers.empty())
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
				<< msg_info("frame_consumer_device requires atleast one consumer."));
		
		needs_clock_ = !std::any_of(consumers.begin(), consumers.end(), std::mem_fn(&frame_consumer::has_sync_clock));
		is_running_ = true;
		display_thread_ = boost::thread([=]{run();});
	}

	~implementation()
	{
		is_running_ = false;
		display_thread_.join();
	}
				
	void run()
	{
		win32_exception::install_handler();
				
		video_sync_clock clock(fmt_);
				
		while(is_running_)
		{
			if(needs_clock_)
				clock.synchronize();
						
			display_frame(frame_processor_->receive());			
		}
	}

	void display_frame(const consumer_frame& frame)
	{		
		typedef std::vector<std::pair<boost::unique_future<void>, frame_consumer_ptr>> sync_container;
		sync_container sync;
		boost::range::transform(consumers_, std::back_inserter(sync), [=](const frame_consumer_ptr& consumer)
		{
			return std::make_pair(consumer->prepare(frame), consumer);
		});
		
		prepared_frames_.push_back(frame);
				
		if(prepared_frames_.size() > 2)
		{	
			boost::range::transform(consumers_, std::back_inserter(sync), [=](const frame_consumer_ptr& consumer)
			{
				return std::make_pair(consumer->display(prepared_frames_.front()), consumer);
			});
			prepared_frames_.pop_front();
		}

		boost::range::for_each(sync, [=](sync_container::value_type& sync)
		{
			try
			{
				sync.first.get();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				boost::range::remove_erase(consumers_, sync.second);
				CASPAR_LOG(warning) << "Removed consumer from frame_consumer_device.";
				if(consumers_.empty())
				{
					CASPAR_LOG(warning) << "No consumers available. Shutting down frame_consumer_device.";
					is_running_ = false;
				}
			}
		});
	}

	std::deque<consumer_frame> prepared_frames_;
		
	boost::thread display_thread_;

	tbb::atomic<bool> is_running_;

	bool needs_clock_;
	std::vector<frame_consumer_ptr> consumers_;

	const video_format_desc& fmt_;

	frame_processor_device_ptr frame_processor_;
};

frame_consumer_device::frame_consumer_device(const frame_processor_device_ptr& frame_processor, const video_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers)
	: impl_(new implementation(frame_processor, format_desc, consumers)){}
}}