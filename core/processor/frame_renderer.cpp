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
		: frame_processor_(frame_processor), index_(0), shader_(format_desc), fbo_(format_desc.width, format_desc.height)
	{	
		auto empty = std::make_shared<internal_frame>(pixel_format_desc());
		output_frame_ = empty;
		std::fill(writing_.begin(), writing_.end(), empty);
		GL(glEnable(GL_POLYGON_STIPPLE));
		GL(glEnable(GL_TEXTURE_2D));
		GL(glEnable(GL_BLEND));
		GL(glDisable(GL_DEPTH_TEST));
		GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
		GL(glViewport(0, 0, format_desc.width, format_desc.height));
	}
				
	internal_frame_ptr render(const internal_frame_ptr& frame)
	{
		if(frame == nullptr)
			return nullptr;

		try
		{
			index_ = (index_ + 1) % 2;
			int next_index = (index_ + 1) % 2;
			
			// 1. Write from page-locked system memory to video memory.
			frame->begin_write();
			writing_[index_] = frame;		
				
			// 3. Map video memory to page-locked system memory.
			output_frame_->end_read();
			
			// Clear framebuffer.
			GL(glClear(GL_COLOR_BUFFER_BIT));	

			// 2. Draw to framebuffer
			writing_[next_index]->draw(shader_);
				
			// Create an output frame
			output_frame_ = std::static_pointer_cast<internal_frame>(frame_processor_.create_frame());
			
			// Read from framebuffer into page-locked memory.
			output_frame_->begin_read();
			output_frame_->get_audio_data() = std::move(writing_[next_index]->get_audio_data());
			
			// Return frames to pool.
			//writing_[next_index]->end_write(); // Is done in frame->reset();
			writing_[next_index] = nullptr;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		return output_frame_;
	}

	size_t index_;

	internal_frame_ptr output_frame_;			

	frame_processor_device& frame_processor_;

	std::array<internal_frame_ptr, 2> writing_;
	
	common::gl::frame_buffer_object fbo_;
	frame_shader shader_;
};
	
frame_renderer::frame_renderer(frame_processor_device& frame_processor, const video_format_desc& format_desc) : impl_(new implementation(frame_processor, format_desc)){}
internal_frame_ptr frame_renderer::render(const internal_frame_ptr& frames){ return impl_->render(frames);}
}}