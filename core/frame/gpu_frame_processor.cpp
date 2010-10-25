#include "../StdAfx.h"

#include "gpu_frame_processor.h"

#include "gpu_frame.h"
#include "frame_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/image/image.h"
#include "../../common/gl/utility.h"

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

namespace caspar {
	
class frame_buffer
{
public:
	frame_buffer(size_t width, size_t height)
	{
		CASPAR_GL_CHECK(glGenTextures(1, &texture_));	

		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));

		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		CASPAR_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL));

		glGenFramebuffersEXT(1, &fbo_);
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
		glBindTexture(GL_TEXTURE_2D, texture_);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture_, 0);
	}

	~frame_buffer()
	{
		glDeleteFramebuffersEXT(1, &fbo_);
	}
		
	GLuint handle() { return fbo_; }
	GLenum attachement() { return GL_COLOR_ATTACHMENT0_EXT; }
	
private:
	GLuint texture_;
	GLuint fbo_;
};
typedef std::shared_ptr<frame_buffer> frame_buffer_ptr;

struct gpu_frame_processor::implementation
{	
	implementation(const frame_format_desc& format_desc) : format_desc_(format_desc)
	{		
		executor_.start();
		executor_.begin_invoke([=]
		{
			context_.reset(new sf::Context());
			context_->SetActive(true);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);			
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glViewport(0, 0, format_desc_.width, format_desc_.height);
			glLoadIdentity();

			input_.resize(2);
			writing_.resize(2);
			fbo_ = std::make_shared<frame_buffer>(format_desc_.width, format_desc_.height);
			output_frame_ = std::make_shared<gpu_frame>(format_desc_.width, format_desc_.height);
			index_ = 0;
		});

		empty_frame_ = create_frame(format_desc.width, format_desc.height);
		common::image::clear(empty_frame_->data(), empty_frame_->size());
		// Fill pipeline length
		for(int n = 0; n < 3; ++n)
			finished_frames_.push(empty_frame_);
	}

	~implementation()
	{
		finished_frames_.push(nullptr);
		executor_.stop();
	}
		
	void pop(gpu_frame_ptr& frame)
	{
		finished_frames_.pop(frame);
	}	
	
	void composite(std::vector<gpu_frame_ptr> frames)
	{
		boost::range::remove_erase(frames, nullptr);
		boost::range::remove_erase(frames, gpu_frame::null());

		executor_.begin_invoke([=]
		{
			try
			{
				index_ = (index_ + 1) % 2;
				int next_index = (index_ + 1) % 2;

				// 2. Start asynchronous DMA transfer to video memory
				// Lock frames and give pointer ownership to OpenGL			
				boost::range::for_each(input_[index_], std::mem_fn(&gpu_frame::write_lock));
				writing_[index_] = input_[index_];	
				input_[index_].clear();
				
				// 1. Copy to page-locked memory
				input_[next_index] = frames;
								
				// 4. Output to external buffer
				if(output_frame_->read_unlock())
					finished_frames_.push(output_frame_);
				output_frame_ = nullptr;
		
				// 3. Draw to framebuffer and start asynchronous DMA transfer to page-locked memory				
				// Clear framebuffer
				glClear(GL_COLOR_BUFFER_BIT);	

				// Draw all frames to framebuffer
				glLoadIdentity();
				boost::range::for_each(writing_[next_index], std::mem_fn(&gpu_frame::draw));
				
				// Create an output frame
				output_frame_ = create_output_frame();
			
				// Read from framebuffer into page-locked memory
				output_frame_->read_lock(GL_COLOR_ATTACHMENT0_EXT);

				// Unlock frames and give back pointer ownership
				boost::range::for_each(writing_[next_index], std::mem_fn(&gpu_frame::write_unlock));
				
				// Mix audio from composite frames into output frame
				std::accumulate(writing_[next_index].begin(), writing_[next_index].end(), output_frame_, mix_audio_safe<gpu_frame_ptr>);	

				// Return frames to pool
				writing_[next_index].clear();
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
		if(!out_frame_pool_.try_pop(frame))				
			frame = std::make_shared<gpu_frame>(format_desc_.width, format_desc_.height);

		return gpu_frame_ptr(frame.get(), [=](gpu_frame*)
		{
			frame->reset();
			out_frame_pool_.push(frame);
		});
	}
		
	gpu_frame_ptr create_frame(size_t width, size_t height)
	{
		size_t key = width | (height << 16);
		auto& pool = input_frame_pools_[key];
		
		gpu_frame_ptr frame;
		if(!pool.try_pop(frame))
		{
			frame = executor_.invoke([=]() -> gpu_frame_ptr
			{
				auto frame = std::make_shared<gpu_frame>(width, height);
				frame->write_unlock();
				return frame;
			});
		}
		
		auto destructor = [=]
		{
			frame->reset();
			input_frame_pools_[key].push(frame);
		};

		return gpu_frame_ptr(frame.get(), [=](gpu_frame*)
		{
			executor_.begin_invoke(destructor);
		});
	}
			
	tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<gpu_frame_ptr>> input_frame_pools_;

	tbb::concurrent_bounded_queue<gpu_frame_ptr> out_frame_pool_;
		
	frame_buffer_ptr fbo_;

	int index_;
	std::vector<std::vector<gpu_frame_ptr>>			input_;
	std::vector<std::vector<gpu_frame_ptr>>			writing_;

	gpu_frame_ptr									output_frame_;
	tbb::concurrent_bounded_queue<gpu_frame_ptr>	finished_frames_;
			
	frame_format_desc format_desc_;
	
	std::unique_ptr<sf::Context> context_;
	common::executor executor_;

	gpu_frame_ptr empty_frame_;
};
	
gpu_frame_processor::gpu_frame_processor(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void gpu_frame_processor::push(const std::vector<gpu_frame_ptr>& frames){ impl_->composite(frames);}
void gpu_frame_processor::pop(gpu_frame_ptr& frame){ impl_->pop(frame);}
gpu_frame_ptr gpu_frame_processor::create_frame(size_t width, size_t height){return impl_->create_frame(width, height);}

}