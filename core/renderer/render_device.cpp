#include "..\StdAfx.h"

#include "render_device.h"

#include "display_device.h"
#include "layer.h"

#include "../frame/frame_format.h"
#include "../frame/gpu_frame_device.h"

#include "../../common/utility/scope_exit.h"
#include "../../common/utility/memory.h"

#include <boost/thread.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/foreach.hpp>

#include <tbb/parallel_for.h>
#include <tbb/mutex.h>
	
namespace caspar { namespace core { namespace renderer {
	
std::vector<gpu_frame_ptr> render_frames(std::map<int, layer>& layers)
{	
	std::vector<gpu_frame_ptr> frames(layers.size(), nullptr);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), 
	[&](const tbb::blocked_range<size_t>& r)
	{
		auto it = layers.begin();
		std::advance(it, r.begin());
		for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			frames[i] = it->second.render_frame();
	});		
	return frames;
}

struct render_device::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers)  
		: display_device_(new display_device(format_desc, consumers)), fmt_(format_desc), frame_processor_(new gpu_frame_device(format_desc))
	{	
		is_running_ = true;
		
		render_thread_ = boost::thread([=]{run();});

		CASPAR_LOG(info) << L"Initialized render_device with " << format_desc;
	}
			
	~implementation()
	{
		is_running_ = false;
		display_device_.reset();
		render_thread_.join();
	}
		
	void run()
	{		
		CASPAR_LOG(info) << L"Started render_device thread";
		win32_exception::install_handler();
		
		while(is_running_)
		{
			try
			{	
				std::vector<gpu_frame_ptr> next_frames;
				{
					tbb::mutex::scoped_lock lock(layers_mutex_);	
					next_frames = render_frames(layers_);
				}
				frame_processor_->push(next_frames);
								
				gpu_frame_ptr frame = frame_processor_->pop();		
				display_device_->display(frame);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				layers_.clear();
				CASPAR_LOG(error) << "Unexpected exception. Cleared layers in render-device";
			}
		}

		CASPAR_LOG(info) << L"Ended render_device thread";
	}

	void load(int render_layer, const frame_producer_ptr& producer, load_option option)
	{
		if(producer->get_frame_format_desc() != fmt_)
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("producer") << msg_info("Invalid frame format"));

		producer->initialize(frame_processor_);
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_[render_layer].load(producer, option);
	}
			
	void pause(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.pause();		
	}

	void play(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.play();		
	}

	void stop(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.stop();
	}

	void clear(int render_layer)
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.clear();		
	}
		
	void clear()
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_.clear();
	}		

	frame_producer_ptr active(int render_layer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		return it != layers_.end() ? it->second.active() : nullptr;
	}
	
	frame_producer_ptr background(int render_layer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		return it != layers_.end() ? it->second.background() : nullptr;
	}
			
	display_device_ptr display_device_;
	boost::thread render_thread_;
		
	frame_format_desc fmt_;
			
	mutable tbb::mutex layers_mutex_;
	std::map<int, layer> layers_;
	
	tbb::atomic<bool> is_running_;	

	gpu_frame_device_ptr frame_processor_;
};

render_device::render_device(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers) 
	: impl_(new implementation(format_desc, consumers)){}
void render_device::load(int render_layer, const frame_producer_ptr& pProducer, load_option option){impl_->load(render_layer, pProducer, option);}
void render_device::pause(int render_layer){impl_->pause(render_layer);}
void render_device::play(int render_layer){impl_->play(render_layer);}
void render_device::stop(int render_layer){impl_->stop(render_layer);}
void render_device::clear(int render_layer){impl_->clear(render_layer);}
void render_device::clear(){impl_->clear();}
frame_producer_ptr render_device::active(int render_layer) const {return impl_->active(render_layer);}
frame_producer_ptr render_device::background(int render_layer) const {return impl_->background(render_layer);}
const frame_format_desc& render_device::get_frame_format_desc() const{return impl_->fmt_;}
}}}
