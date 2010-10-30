#include "..\StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "render_device.h"
#include "layer.h"

#include "../consumer/frame_consumer.h"

#include "../frame/frame_format.h"
#include "../frame/gpu_frame_processor.h"

#include "../../common/utility/scope_exit.h"
#include "../../common/utility/memory.h"

#include <numeric>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/foreach.hpp>

#include <tbb/parallel_for.h>
#include <tbb/mutex.h>

using namespace boost::assign;
	
namespace caspar { namespace core { namespace renderer {
	
std::vector<gpu_frame_ptr> render_frames(std::map<int, layer>& layers)
{	
	std::vector<gpu_frame_ptr> frames(layers.size(), nullptr);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), [&](const tbb::blocked_range<size_t>& r)
	{
		auto it = layers.begin();
		std::advance(it, r.begin());
		for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			frames[i] = it->second.get_frame();
	});					
	return frames;
}

struct render_device::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc, unsigned int index, const std::vector<frame_consumer_ptr>& consumers)  
		: consumers_(consumers), fmt_(format_desc), frame_processor_(new gpu_frame_processor(format_desc)), needs_clock_(false)
	{	
		is_running_ = true;
		if(consumers.empty())
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("render_device requires atleast one consumer"));

		if(std::any_of(consumers.begin(), consumers.end(), [&](const frame_consumer_ptr& pConsumer){ return pConsumer->get_frame_format_desc() != format_desc;}))
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("All consumers must have same frameformat as renderdevice."));
		
		needs_clock_ = !std::any_of(consumers.begin(), consumers.end(), std::mem_fn(&frame_consumer::has_sync_clock));

		frame_buffer_.set_capacity(3);
		display_thread_ = boost::thread([=]{display();});
		render_thread_ = boost::thread([=]{render();});

		CASPAR_LOG(info) << L"Initialized render_device with " << format_desc;
	}
			
	~implementation()
	{
		is_running_ = false;
		frame_buffer_.clear();
		frame_buffer_.push(nullptr);
		render_thread_.join();
		display_thread_.join();
	}
		
	void render()
	{		
		CASPAR_LOG(info) << L"Started render_device::render thread";
		win32_exception::install_handler();
		
		while(is_running_)
		{
			try
			{	
				std::vector<gpu_frame_ptr> next_frames;
				gpu_frame_ptr composite_frame;		

				{
					tbb::mutex::scoped_lock lock(layers_mutex_);	
					next_frames = render_frames(layers_);
				}
				frame_processor_->push(next_frames);
				frame_processor_->pop(composite_frame);	
				frame_buffer_.push(std::move(composite_frame));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				layers_.clear();
				CASPAR_LOG(error) << "Unexpected exception. Cleared layers in render-device";
			}
		}

		CASPAR_LOG(info) << L"Ended render_device::render thread";
	}

	struct video_sync_clock
	{
		video_sync_clock(const frame_format_desc& format_desc)
		{
			period_ = static_cast<long>(get_frame_format_period(format_desc)*1000000.0);
			time_ = boost::posix_time::microsec_clock::local_time();
		}

		void sync_video()
		{
			auto remaining = boost::posix_time::microseconds(period_) - (boost::posix_time::microsec_clock::local_time() - time_);
			if(remaining > boost::posix_time::microseconds(5000))
				boost::this_thread::sleep(remaining - boost::posix_time::microseconds(5000));
			time_ = boost::posix_time::microsec_clock::local_time();
		}

		boost::posix_time::ptime time_;
		long period_;

	};
	
	void display()
	{
		CASPAR_LOG(info) << L"Started render_device::display thread";
		win32_exception::install_handler();
				
		gpu_frame_ptr frame = frame_processor_->create_frame(fmt_.width, fmt_.height);
		common::clear(frame->data(), frame->size());
		std::deque<gpu_frame_ptr> prepared(3, frame);

		video_sync_clock clock(fmt_);
				
		while(is_running_)
		{
			if(needs_clock_)
				clock.sync_video();

			if(!frame_buffer_.try_pop(frame))
			{
				CASPAR_LOG(trace) << "Display Buffer Underrun";
				frame_buffer_.pop(frame);
			}
			if(frame != nullptr)
			{
				send_frame(prepared.front(), frame);
				prepared.push_back(frame);
				prepared.pop_front();
			}
		}
		
		CASPAR_LOG(info) << L"Ended render_device::display thread";
	}

	void send_frame(const gpu_frame_ptr& prepared_frame, const gpu_frame_ptr& next_frame)
	{
		BOOST_FOREACH(const frame_consumer_ptr& consumer, consumers_)
		{
			try
			{
				consumer->prepare(next_frame); // Could block
				consumer->display(prepared_frame); // Could block
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				boost::range::remove_erase(consumers_, consumer);
				CASPAR_LOG(warning) << "Removed consumer from render-device.";
				if(consumers_.empty())
				{
					CASPAR_LOG(warning) << "No consumers available. Shutting down render-device.";
					is_running_ = false;
				}
			}
		}
	}
		
	void load(int exLayer, const frame_producer_ptr& producer, load_option option)
	{
		if(producer->get_frame_format_desc() != fmt_)
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("pProducer") << msg_info("Invalid frame format"));

		producer->initialize(frame_processor_);
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_[exLayer].load(producer, option);
	}
			
	void pause(int exLayer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.pause();		
	}

	void play(int exLayer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.play();		
	}

	void stop(int exLayer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.stop();
	}

	void clear(int exLayer)
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.clear();		
	}
		
	void clear()
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_.clear();
	}		

	frame_producer_ptr active(int exLayer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		return it != layers_.end() ? it->second.active() : nullptr;
	}
	
	frame_producer_ptr background(int exLayer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		return it != layers_.end() ? it->second.background() : nullptr;
	}
			
	boost::thread render_thread_;
	boost::thread display_thread_;
		
	frame_format_desc fmt_;
	tbb::concurrent_bounded_queue<gpu_frame_ptr> frame_buffer_;
	
	std::vector<frame_consumer_ptr> consumers_;
	
	mutable tbb::mutex layers_mutex_;
	std::map<int, layer> layers_;
	
	tbb::atomic<bool> is_running_;	

	gpu_frame_processor_ptr frame_processor_;

	bool needs_clock_;
};

render_device::render_device(const frame_format_desc& format_desc, unsigned int index, const std::vector<frame_consumer_ptr>& consumers) 
	: impl_(new implementation(format_desc, index, consumers)){}
void render_device::load(int exLayer, const frame_producer_ptr& pProducer, load_option option){impl_->load(exLayer, pProducer, option);}
void render_device::pause(int exLayer){impl_->pause(exLayer);}
void render_device::play(int exLayer){impl_->play(exLayer);}
void render_device::stop(int exLayer){impl_->stop(exLayer);}
void render_device::clear(int exLayer){impl_->clear(exLayer);}
void render_device::clear(){impl_->clear();}
frame_producer_ptr render_device::active(int exLayer) const {return impl_->active(exLayer);}
frame_producer_ptr render_device::background(int exLayer) const {return impl_->background(exLayer);}
const frame_format_desc& render_device::get_frame_format_desc() const{return impl_->fmt_;}
}}}
