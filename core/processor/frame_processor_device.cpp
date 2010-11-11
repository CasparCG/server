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
		boost::promise<frame_ptr> promise;
		active_frame_ = promise.get_future();
		promise.set_value(nullptr);

		input_.set_capacity(2);
		output_.set_capacity(2);
		executor_.start();
		executor_.invoke([=]
		{
			ogl_context_.reset(new sf::Context());
			ogl_context_->SetActive(true);
			GL(glEnable(GL_POLYGON_STIPPLE));
			GL(glEnable(GL_TEXTURE_2D));
			GL(glEnable(GL_BLEND));
			GL(glDisable(GL_DEPTH_TEST));
			GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
			GL(glClearColor(0.0, 0.0, 0.0, 0.0));
			GL(glViewport(0, 0, format_desc.width, format_desc.height));
			glLoadIdentity();   

			renderer_ = std::make_shared<frame_renderer>(*self, format_desc);
		});
	}

	~implementation()
	{
		executor_.stop();
	}
					
	frame_ptr create_frame(const pixel_format_desc& desc, void* tag)
	{
		size_t key = reinterpret_cast<size_t>(tag);
		auto& pool = writing_pools_[key];
		
		frame_ptr my_frame;
		if(!pool.try_pop(my_frame))
		{
			my_frame = executor_.invoke([&]
			{
				return std::shared_ptr<frame>(new frame(desc));
			});
		}
		
		auto destructor = [=]
		{
			my_frame->reset();
			writing_pools_[key].push(my_frame);
		};

		return frame_ptr(my_frame.get(), [=](frame*)							
		{
			executor_.begin_invoke(destructor);
		});
	}

	void release_tag(void* tag)
	{
		writing_pools_[reinterpret_cast<size_t>(tag)].clear();
	}
	
	void send(const frame_ptr& input_frame)
	{			
		input_.push(input_frame); // Block if there are too many frames in pipeline
		executor_.begin_invoke([=]
		{
			try
			{
				frame_ptr output_frame;
				input_.pop(output_frame);
				if(output_frame != nullptr)
					output_.push(renderer_->render(output_frame));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});	
	}

	void receive(frame_ptr& output_frame)
	{
		if(!output_.try_pop(output_frame))
		{
			if(underrun_count_ == 0)			
				CASPAR_LOG(trace) << "Frame Processor Underrun has STARTED.";
			
			++underrun_count_;
			output_.pop(output_frame);
		}		
		else if(underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "Frame Processor Underrun has ENDED with " << underrun_count_ << " ticks.";
			underrun_count_ = 0;
		}
	}

	video_format_desc fmt_;
				
	typedef tbb::concurrent_bounded_queue<frame_ptr> frame_queue;
	tbb::concurrent_unordered_map<size_t, frame_queue> writing_pools_;
	frame_queue reading_pool_;	
				
	std::unique_ptr<sf::Context> ogl_context_;

	boost::unique_future<frame_ptr> active_frame_;
	
	common::executor executor_;
	
	frame_queue input_;
	frame_queue output_;	

	frame_renderer_ptr renderer_;

	long underrun_count_;
};
	
#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

frame_processor_device::frame_processor_device(const video_format_desc& format_desc) 
	: impl_(new implementation(this, format_desc)){}
frame_ptr frame_processor_device::create_frame(const  pixel_format_desc& desc, void* tag){return impl_->create_frame(desc, tag);}
void frame_processor_device::release_tag(void* tag){impl_->release_tag(tag);}
void frame_processor_device::send(const frame_ptr& frame){impl_->send(frame);}
void frame_processor_device::receive(frame_ptr& frame){impl_->receive(frame);}
const video_format_desc frame_processor_device::get_video_format_desc() const { return impl_->fmt_;}
frame_ptr frame_processor_device::create_frame(size_t width, size_t height, void* tag)
{
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes[0] = pixel_format_desc::plane(width, height, 4);
	return create_frame(desc, tag);
}
			
frame_ptr frame_processor_device::create_frame(void* tag)
{
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes[0] = pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4);
	return create_frame(desc, tag);
}
}}