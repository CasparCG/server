#include "../StdAfx.h"

#include "gpu_frame_processor.h"

#include "gpu_frame.h"
#include "gpu_composite_frame.h"
#include "frame_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/utility/memory.h"
#include "../../common/gl/gl_check.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#include <boost/range.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>
#include <unordered_map>
#include <numeric>

namespace caspar { namespace core {
	
struct gpu_frame_processor::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc) 
		: format_desc_(format_desc), index_(0)
	{		
		input_.set_capacity(2);
		executor_.start();
		executor_.begin_invoke([=]
		{
			ogl_context_.reset(new sf::Context());
			ogl_context_->SetActive(true);
			GL(glEnable(GL_POLYGON_STIPPLE));
			GL(glEnable(GL_TEXTURE_2D));
			GL(glEnable(GL_BLEND));
			GL(glDisable(GL_DEPTH_TEST));
			GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
			GL(glClearColor(0.0, 0.0, 0.0, 0.0));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			glLoadIdentity();   

			fbo_.create(format_desc_.width, format_desc_.height);
			fbo_.bind_pixel_source();

			writing_.resize(2, std::make_shared<gpu_composite_frame>());

			// Fill pipeline
			for(int n = 0; n < 2; ++n)
				composite(std::vector<gpu_frame_ptr>());
		});
	}

	~implementation()
	{
		executor_.stop();
	}
			
	void composite(std::vector<gpu_frame_ptr> frames)
	{
		boost::range::remove_erase(frames, nullptr);
		boost::range::remove_erase(frames, gpu_frame::null());
		auto composite_frame = std::make_shared<gpu_composite_frame>();
		boost::range::for_each(frames, std::bind(&gpu_composite_frame::add, 
													composite_frame, 
													std::placeholders::_1));

		input_.push(composite_frame);
		executor_.begin_invoke([=]
		{
			try
			{
				gpu_frame_ptr frame;
				input_.pop(frame);

				index_ = (index_ + 1) % 2;
				int next_index = (index_ + 1) % 2;

				// 1. Start asynchronous DMA transfer to video memory.
				writing_[index_] = std::move(frame);		
				// Lock frame and give pointer ownership to OpenGL.
				writing_[index_]->begin_write();
				
				// 3. Output to external buffer.
				if(output_frame_)
				{	
					output_frame_->end_read();
					output_.push(output_frame_);
				}

				// Clear framebuffer.
				GL(glClear(GL_COLOR_BUFFER_BIT));	

				// 2. Draw to framebuffer and start asynchronous DMA transfer 
				// to page-locked memory.
				writing_[next_index]->draw();
				
				// Create an output frame
				auto temp_frame = create_output_frame();
			
				// Read from framebuffer into page-locked memory.
				temp_frame->begin_read();
				temp_frame->audio_data() = std::move(writing_[next_index]->audio_data());

				output_frame_ = temp_frame;

				// Return frames to pool.
				writing_[next_index]->end_write();
				writing_[next_index] = nullptr;
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});	
	}

	gpu_frame_ptr create_output_frame()
	{
		gpu_frame_ptr frame;
		if(!reading_pool_.try_pop(frame))
			frame.reset(new gpu_frame(format_desc_.width, format_desc_.height));

		return gpu_frame_ptr(frame.get(), [=](gpu_frame*)
		{
			frame->reset();
			reading_pool_.push(frame);
		});
	}
			
	gpu_frame_ptr create_frame(size_t width, size_t height)
	{
		size_t key = width | (height << 16);
		auto& pool = writing_pools_[key];
		
		gpu_frame_ptr frame;
		if(!pool.try_pop(frame))
		{
			frame = executor_.invoke([&]
			{
				return std::shared_ptr<gpu_frame>(new gpu_frame(width, height));
			});
		}
		
		auto destructor = [=]
		{
			frame->reset();
			writing_pools_[key].push(frame);
		};

		return gpu_frame_ptr(frame.get(), [=](gpu_frame*)							
		{
			executor_.begin_invoke(destructor);
		});
	}
	
	void pop(gpu_frame_ptr& frame)
	{
		output_.pop(frame);
	}
			
	typedef tbb::concurrent_bounded_queue<gpu_frame_ptr> gpu_frame_queue;
	tbb::concurrent_unordered_map<size_t, gpu_frame_queue> writing_pools_;
	gpu_frame_queue reading_pool_;	

	gpu_frame_queue input_;
	std::vector<gpu_frame_ptr> writing_;
	gpu_frame_queue output_;	

	size_t index_;

	gpu_frame_ptr output_frame_;			
	frame_format_desc format_desc_;
	
	std::unique_ptr<sf::Context> ogl_context_;
	
	common::executor executor_;

	common::gl::frame_buffer_object fbo_;
};
	
gpu_frame_processor::gpu_frame_processor(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void gpu_frame_processor::push(const std::vector<gpu_frame_ptr>& frames){ impl_->composite(frames);}
void gpu_frame_processor::pop(gpu_frame_ptr& frame){impl_->pop(frame);}
gpu_frame_ptr gpu_frame_processor::create_frame(size_t width, size_t height){return impl_->create_frame(width, height);}

}}