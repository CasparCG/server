#include "../StdAfx.h"

#include "frame_renderer.h"

#include "../processor/frame.h"

#include "../format/video_format.h"
#include "../processor/frame_processor_device.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>

#include <tbb/concurrent_queue.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct frame_renderer::implementation : boost::noncopyable
{	
	implementation(frame_processor_device& frame_processor, const video_format_desc& format_desc) 
		: frame_processor_(frame_processor), format_desc_(format_desc), index_(0), shader_(std::make_shared<frame_shader>(format_desc)), 
			writing_(2, frame::empty()), output_frame_(frame::empty())
	{	
		fbo_.create(format_desc_.width, format_desc_.height);
		fbo_.bind_pixel_source();
	}
				
	frame_ptr render(const frame_ptr& frame)
	{
		if(frame == nullptr)
			return nullptr;

		frame_ptr result;
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
			auto temp_frame = frame_processor_.create_frame();
			
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
		return result == frame::empty() ? nullptr : result;;
	}

	size_t index_;

	frame_ptr output_frame_;			
	video_format_desc format_desc_;
	frame_processor_device& frame_processor_;

	std::vector<frame_ptr> writing_;
	
	common::gl::frame_buffer_object fbo_;
	frame_shader_ptr shader_;
};
	
frame_renderer::frame_renderer(frame_processor_device& frame_processor, const video_format_desc& format_desc) : impl_(new implementation(frame_processor, format_desc)){}
frame_ptr frame_renderer::render(const frame_ptr& frames){ return impl_->render(frames);}
}}