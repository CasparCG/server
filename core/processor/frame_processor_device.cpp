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
		: fmt_(format_desc), output_underrun_count_(0), input_underrun_count_(0)
	{		
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
		stop();
	}

	void stop()
	{
		output_.clear();
		input_.clear();
		input_.push(nullptr);
		output_.push(nullptr);
		executor_.stop();
	}

	frame_ptr create_frame(const pixel_format_desc& desc)
	{
		size_t key = pixel_format_desc::hash(desc);
		auto& pool = writing_pools_[key];
		
		frame_ptr my_frame;
		if(!pool.try_pop(my_frame))		
			my_frame = executor_.invoke([&]{return std::shared_ptr<frame>(new frame(desc));});		
		
		auto destructor = [=]
		{
			my_frame->reset();
			writing_pools_[key].push(my_frame);
		};

		return frame_ptr(my_frame.get(), [=](frame*){executor_.begin_invoke(destructor);});
	}
		
	void send(const frame_ptr& input_frame)
	{			
		if(input_frame == nullptr)
			return;

		input_.push(input_frame); // Block if there are too many frames in pipeline
		executor_.begin_invoke([=]
		{
			try
			{
				output_.push(renderer_->render(input_frame));
				frame_ptr dummy;
				input_.try_pop(dummy);
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
			if(input_.empty())
			{
				if(input_underrun_count_ == 0)			
					CASPAR_LOG(trace) << "### Frame Processor Input Underrun has STARTED. ###";
			
				++input_underrun_count_;
			}
			else 
			{
				if(output_underrun_count_ == 0)			
					CASPAR_LOG(trace) << "### Frame Processor Output Underrun has STARTED. ###";
			
				++output_underrun_count_;
			}

			output_.pop(output_frame);
		}	
		else if(input_underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Frame Processor Input Underrun has ENDED with " << output_underrun_count_ << " ticks. ###";
			input_underrun_count_ = 0;
		}	
		else if(output_underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Frame Processor Output Underrun has ENDED with " << output_underrun_count_ << " ticks. ###";
			output_underrun_count_ = 0;
		}
	}

	video_format_desc fmt_;
				
	typedef tbb::concurrent_bounded_queue<frame_ptr> frame_queue;
	tbb::concurrent_unordered_map<size_t, frame_queue> writing_pools_;
	frame_queue reading_pool_;	
				
	std::unique_ptr<sf::Context> ogl_context_;
	
	common::executor executor_;
	
	frame_queue input_;
	frame_queue output_;	

	frame_renderer_ptr renderer_;

	long output_underrun_count_;
	long input_underrun_count_;
};
	
#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

frame_processor_device::frame_processor_device(const video_format_desc& format_desc) 
	: impl_(new implementation(this, format_desc)){}
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
void frame_processor_device::stop(){impl_->stop();}
}}