#include "../StdAfx.h"

#include "frame_processor_device.h"

#include "frame_renderer.h"
#include "frame.h"
#include "composite_frame.h"

#include "../format/video_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/gl/utility.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct frame_processor_device::implementation : boost::noncopyable
{	
	implementation(frame_processor_device* self, const video_format_desc& format_desc) 
		: fmt_(format_desc), underrun_count_(0)
	{		
		output_.set_capacity(4);
		executor_.start();
		executor_.invoke([=]
		{
			ogl_context_.reset(new sf::Context());
			ogl_context_->SetActive(true);

			renderer_.reset(new frame_renderer(*self, format_desc));
		});
	}

	~implementation()
	{
		executor_.stop(); // Wait for executor before destroying anything.
	}
	
	void clear()
	{
		output_.clear();
	}

	frame_ptr create_frame(const pixel_format_desc& desc)
	{
		int key = pixel_format_desc::hash(desc);
		auto& pool = frame_pools_[key];
		
		frame_ptr my_frame;
		if(!pool.try_pop(my_frame))		
			my_frame = executor_.invoke([&]{return std::shared_ptr<frame>(new frame(desc));});		
		
		auto destructor = [=]
		{
			my_frame->reset();
			frame_pools_[key].push(my_frame);
		};

		return frame_ptr(my_frame.get(), [=](frame*){executor_.begin_invoke(destructor);});
	}
		
	void send(const frame_ptr& input_frame)
	{			
		if(input_frame == nullptr)
			return;
				
		auto future = executor_.begin_invoke([=]{return renderer_->render(input_frame);});	
		output_.push(std::move(future)); // Blocks
	}

	void receive(frame_ptr& output_frame)
	{
		boost::shared_future<frame_ptr> future;

		if(!output_.try_pop(future))
		{
			if(underrun_count_++ == 0)			
				CASPAR_LOG(trace) << "### Frame Processor Output Underrun has STARTED. ###";
			
			output_.pop(future);
		}	
		else if(underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Frame Processor Output Underrun has ENDED with " << underrun_count_ << " ticks. ###";
			underrun_count_ = 0;
		}

		output_frame = future.get();
	}
					
	std::unique_ptr<sf::Context> ogl_context_;
	
	common::executor executor_;	

	std::unique_ptr<frame_renderer> renderer_;
				
	tbb::concurrent_bounded_queue<boost::shared_future<frame_ptr>> output_;	
	tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<frame_ptr>> frame_pools_;
	
	video_format_desc fmt_;
	long underrun_count_;
};
	
#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

frame_processor_device::frame_processor_device(const video_format_desc& format_desc) : impl_(new implementation(this, format_desc)){}
frame_ptr frame_processor_device::create_frame(const  pixel_format_desc& desc){return impl_->create_frame(desc);}
void frame_processor_device::send(const frame_ptr& frame){impl_->send(frame);}
void frame_processor_device::receive(frame_ptr& frame){impl_->receive(frame);}
const video_format_desc frame_processor_device::get_video_format_desc() const { return impl_->fmt_;}

frame_ptr frame_processor_device::create_frame(size_t width, size_t height)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes[0] = pixel_format_desc::plane(width, height, 4);
	return create_frame(desc);
}
			
frame_ptr frame_processor_device::create_frame()
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes[0] = pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4);
	return create_frame(desc);
}
void frame_processor_device::clear(){impl_->clear();}
}}