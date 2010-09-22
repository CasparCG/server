#include "..\StdAfx.h"

#include "render_device.h"
#include "layer.h"

#include "../consumer/frame_consumer.h"

#include "../frame/system_frame.h"
#include "../frame/format.h"
#include "../frame/algorithm.h"
#include "../frame/gpu/frame_processor.h"

#include "../../common/utility/scope_exit.h"
#include "../../common/image/image.h"

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
	
namespace caspar { namespace renderer {
	
std::vector<frame_ptr> render_frames(std::map<int, layer>& layers)
{	
	std::vector<frame_ptr> frames(layers.size(), nullptr);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), [&](const tbb::blocked_range<size_t>& r)
	{
		auto it = layers.begin();
		std::advance(it, r.begin());
		for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			frames[i] = it->second.get_frame();
	});					
	boost::range::remove_erase(frames, nullptr);
	boost::range::remove_erase_if(frames, [](const frame_const_ptr& frame) { return *frame == *frame::null();});
	
	return frames;
}

struct render_device::implementation : boost::noncopyable
{	
	implementation(const caspar::frame_format_desc& format_desc, unsigned int index, const std::vector<frame_consumer_ptr>& consumers)  
		: consumers_(consumers), format_desc_(format_desc), gpu_frame_processor_(format_desc)
	{	
		is_running_ = true;
		if(consumers.empty())
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("render_device requires atleast one consumer"));

		if(std::any_of(consumers.begin(), consumers.end(), [&](const frame_consumer_ptr& pConsumer){ return pConsumer->get_frame_format_desc() != format_desc;}))
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("consumer") 
														<< msg_info("All consumers must have same frameformat as renderdevice."));
		
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
		CASPAR_LOG(info) << L"Started render_device::render Thread";
		win32_exception::install_handler();
		
		std::vector<frame_ptr> current_frames;

		while(is_running_)
		{
			try
			{	
				std::vector<frame_ptr> next_frames;
				frame_ptr composite_frame;		

				{
					tbb::mutex::scoped_lock lock(layers_mutex_);	
					next_frames = render_frames(layers_);
				}
				gpu_frame_processor_.composite(next_frames);
				composite_frame = gpu_frame_processor_.get_frame();

				current_frames = std::move(next_frames);		
				frame_buffer_.push(std::move(composite_frame));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				layers_.clear();
				CASPAR_LOG(error) << "Unexpected exception. Cleared layers in render-device";
			}
		}

		CASPAR_LOG(info) << L"Ended render_device::render Thread";
	}
	
	void display()
	{
		CASPAR_LOG(info) << L"Started render_device::display Thread";
		win32_exception::install_handler();
				
		frame_ptr frame = clear_frame(std::make_shared<system_frame>(format_desc_));
		std::deque<frame_ptr> prepared(3, frame);
				
		while(is_running_)
		{
			if(!frame_buffer_.try_pop(frame))
			{
#ifdef CASPAR_TRACE_UNDERFLOW
				CASPAR_LOG(trace) << "Display Buffer Underrun";
				frame_buffer_.pop(frame);
#endif
			}
			if(frame != nullptr)
			{
				send_frame(prepared.front(), frame);
				prepared.push_back(frame);
				prepared.pop_front();
			}
		}
		
		CASPAR_LOG(info) << L"Ended render_device::display Thread";
	}

	void send_frame(const frame_ptr& pPreparedFrame, const frame_ptr& pNextFrame)
	{
		BOOST_FOREACH(const frame_consumer_ptr& consumer, consumers_)
		{
			try
			{
				consumer->prepare(pNextFrame); // Could block
				consumer->display(pPreparedFrame); // Could block
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
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_[exLayer].load(producer, option);
	}
			
	void play(int exLayer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.play();	
		else
			BOOST_THROW_EXCEPTION(invalid_layer_index() << arg_name_info("exLayer"));
	}

	void stop(int exLayer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.stop();
		else
			BOOST_THROW_EXCEPTION(invalid_layer_index() << arg_name_info("exLayer"));
	}

	void clear(int exLayer)
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(exLayer);
		if(it != layers_.end())
			it->second.clear();	
		else
			BOOST_THROW_EXCEPTION(invalid_layer_index() << arg_name_info("exLayer"));
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
		
	caspar::frame_format_desc format_desc_;
	tbb::concurrent_bounded_queue<frame_ptr> frame_buffer_;
	
	std::vector<frame_consumer_ptr> consumers_;
	
	mutable tbb::mutex layers_mutex_;
	std::map<int, layer> layers_;
	
	tbb::atomic<bool> is_running_;	

	gpu::frame_processor gpu_frame_processor_;
};

render_device::render_device(const caspar::frame_format_desc& format_desc, unsigned int index, const std::vector<frame_consumer_ptr>& consumers) 
	: impl_(new implementation(format_desc, index, consumers)){}
void render_device::load(int exLayer, const frame_producer_ptr& pProducer, load_option option){impl_->load(exLayer, pProducer, option);}
void render_device::play(int exLayer){impl_->play(exLayer);}
void render_device::stop(int exLayer){impl_->stop(exLayer);}
void render_device::clear(int exLayer){impl_->clear(exLayer);}
void render_device::clear(){impl_->clear();}
frame_producer_ptr render_device::active(int exLayer) const {return impl_->active(exLayer);}
frame_producer_ptr render_device::background(int exLayer) const {return impl_->background(exLayer);}
const frame_format_desc& render_device::frame_format_desc() const{return impl_->format_desc_;}

}}

