#include "..\StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "render_device.h"

#include "display_device.h"
#include "layer.h"

#include "../frame/frame_format.h"
#include "../frame/gpu_frame_processor.h"

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
		: display_device_(new display_device(format_desc, consumers)), fmt_(format_desc), frame_processor_(new gpu_frame_processor(format_desc))
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
								
				gpu_frame_ptr frame;		
				frame_processor_->pop(frame);
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
			
	display_device_ptr display_device_;
	boost::thread render_thread_;
		
	frame_format_desc fmt_;
			
	mutable tbb::mutex layers_mutex_;
	std::map<int, layer> layers_;
	
	tbb::atomic<bool> is_running_;	

	gpu_frame_processor_ptr frame_processor_;
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
