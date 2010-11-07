#include "../StdAfx.h"

#include "gpu_frame_renderer.h"

#include "gpu_frame.h"

#include "frame_format.h"
#include "frame_factory.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>

#include <tbb/concurrent_queue.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct gpu_frame_renderer::implementation : boost::noncopyable
{	
	implementation(frame_factory& factory, const frame_format_desc& format_desc) 
		: factory_(factory), format_desc_(format_desc), index_(0), shader_(std::make_shared<gpu_frame_shader>(format_desc)), 
			writing_(2, gpu_frame::null()), output_frame_(gpu_frame::null())
	{	
		fbo_.create(format_desc_.width, format_desc_.height);
		fbo_.bind_pixel_source();
	}
				
	gpu_frame_ptr render(const gpu_frame_ptr& frame)
	{
		gpu_frame_ptr result;
		try
		{
			index_ = (index_ + 1) % 2;
			int next_index = (index_ + 1) % 2;

			// 1. Start asynchronous DMA transfer to video memory.
			writing_[index_] = std::move(frame);		
			// Lock frame and give pointer ownership to OpenGL.
			writing_[index_]->begin_write();
				
			// 3. Output to external buffer.
			output_frame_->end_read();
			result = output_frame_;
			
			// Clear framebuffer.
			GL(glClear(GL_COLOR_BUFFER_BIT));	

			// 2. Draw to framebuffer and start asynchronous DMA transfer 
			// to page-locked memory.
			writing_[next_index]->draw(shader_);
				
			// Create an output frame
			auto temp_frame = factory_.create_frame(format_desc_, this);
			
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
		return result == gpu_frame::null() ? nullptr : result;;
	}

	size_t index_;

	gpu_frame_ptr output_frame_;			
	frame_format_desc format_desc_;
	frame_factory& factory_;

	std::vector<gpu_frame_ptr> writing_;
	
	common::gl::frame_buffer_object fbo_;
	gpu_frame_shader_ptr shader_;
};
	
gpu_frame_renderer::gpu_frame_renderer(frame_factory& factory, const frame_format_desc& format_desc) : impl_(new implementation(factory, format_desc)){}
gpu_frame_ptr gpu_frame_renderer::render(const gpu_frame_ptr& frames){ return impl_->render(frames);}
}}