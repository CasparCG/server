#include "../StdAfx.h"

#include "gpu_frame_processor.h"

#include "gpu_frame.h"
#include "gpu_composite_frame.h"
#include "frame_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/utility/memory.h"
#include "../../common/gl/gl_check.h"

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
	
class frame_buffer : boost::noncopyable
{
public:
	frame_buffer(size_t width, size_t height)
	{
		CASPAR_GL_CHECK(glGenTextures(1, &texture_));	

		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));

		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		//CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		//CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

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

struct gpu_frame_processor::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc) : format_desc_(format_desc)
	{		
		input_.set_capacity(2);
		executor_.start();
		executor_.begin_invoke([=]
		{
			ogl_context_.reset(new sf::Context());
			ogl_context_->SetActive(true);
			CASPAR_GL_CHECK(glEnable(GL_POLYGON_STIPPLE));
			CASPAR_GL_CHECK(glEnable(GL_TEXTURE_2D));
			CASPAR_GL_CHECK(glEnable(GL_BLEND));
			CASPAR_GL_CHECK(glDisable(GL_DEPTH_TEST));
			CASPAR_GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
			CASPAR_GL_CHECK(glClearColor(0.0, 0.0, 0.0, 0.0));
			CASPAR_GL_CHECK(glViewport(0, 0, format_desc_.width, format_desc_.height));
			glLoadIdentity();        
			
			reading_.resize(2, std::make_shared<gpu_composite_frame>(format_desc_.width, format_desc_.height));
			writing_.resize(2, std::make_shared<gpu_composite_frame>(format_desc_.width, format_desc_.height));
			fbo_ = std::make_shared<frame_buffer>(format_desc_.width, format_desc_.height);
			output_frame_ = std::make_shared<gpu_frame>(format_desc_.width, format_desc_.height);
			index_ = 0;
		});
		composite(std::vector<gpu_frame_ptr>());
		composite(std::vector<gpu_frame_ptr>());
		composite(std::vector<gpu_frame_ptr>());
	}

	~implementation()
	{
		executor_.stop();
	}
			
	void composite(std::vector<gpu_frame_ptr> frames)
	{
		boost::range::remove_erase(frames, nullptr);
		boost::range::remove_erase(frames, gpu_frame::null());
		auto composite_frame = std::make_shared<gpu_composite_frame>(format_desc_.width, format_desc_.height);
		boost::range::for_each(frames, std::bind(&gpu_composite_frame::add, composite_frame, std::placeholders::_1));
		input_.push(composite_frame);

		executor_.begin_invoke([=]
		{
			try
			{
				gpu_composite_frame_ptr frame;
				input_.pop(frame);

				index_ = (index_ + 1) % 2;
				int next_index = (index_ + 1) % 2;

				// 2. Start asynchronous DMA transfer to video memory
				// Lock frames and give pointer ownership to OpenGL		
				writing_[index_] = std::move(reading_[index_]);		
				writing_[index_]->write_lock();
				
				// 1. Copy to page-locked memory
				reading_[next_index] = std::move(frame);
								
				// 4. Output to external buffer
				if(output_frame_->read_unlock())
					output_.push(output_frame_);
		
				// 3. Draw to framebuffer and start asynchronous DMA transfer to page-locked memory				
				// Clear framebuffer
				glClear(GL_COLOR_BUFFER_BIT);	
				writing_[next_index]->draw();
				
				// Create an output frame
				output_frame_ = create_frame(format_desc_.width, format_desc_.height);
			
				// Read from framebuffer into page-locked memory
				output_frame_->read_lock(GL_COLOR_ATTACHMENT0_EXT);
				output_frame_->audio_data() = std::move(writing_[next_index]->audio_data());

				// Return frames to pool
				writing_[next_index] = nullptr;
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});	
	}
			
	gpu_frame_ptr create_frame(size_t width, size_t height)
	{
		size_t key = width | (height << 16);
		auto& pool = reading_frame_pools_[key];
		
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
			frame->write_unlock();
			frame->reset();
			reading_frame_pools_[key].push(frame);
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
			
	tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<gpu_frame_ptr>> reading_frame_pools_;

	frame_buffer_ptr fbo_;

	tbb::concurrent_bounded_queue<gpu_composite_frame_ptr> input_;
	tbb::concurrent_bounded_queue<gpu_frame_ptr> output_;	

	size_t index_;
	std::vector<gpu_composite_frame_ptr> reading_;
	std::vector<gpu_composite_frame_ptr> writing_;

	gpu_frame_ptr output_frame_;			
	frame_format_desc format_desc_;
	
	std::unique_ptr<sf::Context> ogl_context_;
	
	common::executor executor_;
};
	
gpu_frame_processor::gpu_frame_processor(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void gpu_frame_processor::push(const std::vector<gpu_frame_ptr>& frames){ impl_->composite(frames);}
void gpu_frame_processor::pop(gpu_frame_ptr& frame){impl_->pop(frame);}
gpu_frame_ptr gpu_frame_processor::create_frame(size_t width, size_t height){return impl_->create_frame(width, height);}

}}