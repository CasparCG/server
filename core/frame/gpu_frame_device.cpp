#include "../StdAfx.h"

#include "gpu_frame_device.h"

#include "gpu_frame_renderer.h"
#include "gpu_frame.h"
#include "gpu_composite_frame.h"

#include "frame_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/gl/utility.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/thread.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct gpu_frame_device::implementation : boost::noncopyable
{	
	implementation(gpu_frame_device* self, const frame_format_desc& format_desc) 
	{		
		input_.set_capacity(2);
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

			renderer_ = std::make_shared<gpu_frame_renderer>(*self, format_desc);
		});
	}

	~implementation()
	{
		executor_.stop();
	}
			
	void push(const std::vector<gpu_frame_ptr>& frames)
	{
		auto composite_frame = std::make_shared<gpu_composite_frame>();
		boost::range::for_each(frames, std::bind(&gpu_composite_frame::add, composite_frame, std::placeholders::_1));

		input_.push(composite_frame);
		executor_.begin_invoke([=]
		{
			try
			{
				gpu_frame_ptr frame;
				input_.pop(frame);
				output_.push(renderer_->render(frame));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});	
	}
	
	gpu_frame_ptr pop()
	{
		gpu_frame_ptr frame;
		output_.pop(frame);
		return frame;
	}
				
	gpu_frame_ptr do_create_frame(size_t key, const std::function<gpu_frame*()>& constructor)
	{
		auto& pool = writing_pools_[key];
		
		gpu_frame_ptr frame;
		if(!pool.try_pop(frame))
		{
			frame = executor_.invoke([&]
			{
				return std::shared_ptr<gpu_frame>(constructor());
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

	gpu_frame_ptr create_frame(size_t width, size_t height, void* tag)
	{
		return do_create_frame(reinterpret_cast<size_t>(tag), [&]
		{
			return new gpu_frame(width, height);
		});
	}
	
	gpu_frame_ptr create_frame(const gpu_frame_desc& desc, void* tag)
	{
		return do_create_frame(reinterpret_cast<size_t>(tag), [&]
		{
			return new gpu_frame(desc);
		});
	}

	void release_frames(void* tag)
	{
		writing_pools_[reinterpret_cast<size_t>(tag)].clear();
	}
				
	typedef tbb::concurrent_bounded_queue<gpu_frame_ptr> gpu_frame_queue;
	tbb::concurrent_unordered_map<size_t, gpu_frame_queue> writing_pools_;
	gpu_frame_queue reading_pool_;	

	gpu_frame_queue input_;
	gpu_frame_queue output_;	
		
	std::unique_ptr<sf::Context> ogl_context_;
	
	common::executor executor_;

	gpu_frame_renderer_ptr renderer_;
};
	
#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

gpu_frame_device::gpu_frame_device(const frame_format_desc& format_desc) : impl_(new implementation(this, format_desc)){}
void gpu_frame_device::push(const std::vector<gpu_frame_ptr>& frames){ impl_->push(frames);}
gpu_frame_ptr gpu_frame_device::pop(){return impl_->pop();}
gpu_frame_ptr gpu_frame_device::create_frame(size_t width, size_t height, void* tag){return impl_->create_frame(width, height, tag);}
gpu_frame_ptr gpu_frame_device::create_frame(const  gpu_frame_desc& desc, void* tag){return impl_->create_frame(desc, tag);}
void gpu_frame_device::release_frames(void* tag){impl_->release_frames(tag);}
}}