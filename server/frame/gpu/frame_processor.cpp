#include "../../StdAfx.h"

#include "frame_processor.h"

#include "../frame.h"
#include "../format.h"
#include "../algorithm.h"
#include "../system_frame.h"

#include "../../../common/exception/exceptions.h"
#include "../../../common/image/image.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#include <boost/range.hpp>

#include <functional>

namespace caspar { namespace gpu {
	
class fullscreen_quad
{
public:

	fullscreen_quad()
	{
		dlist_ = glGenLists(1);

		glNewList(dlist_, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f,	 0.0f); glVertex2f(-1.0f, -1.0f);
				glTexCoord2f(1.0f,	 0.0f);	glVertex2f( 1.0f, -1.0f);
				glTexCoord2f(1.0f,	 1.0f);	glVertex2f( 1.0f,  1.0f);
				glTexCoord2f(0.0f,	 1.0f);	glVertex2f(-1.0f,  1.0f);
			glEnd();	
		glEndList();
	}

	void draw() const
	{
		glCallList(dlist_);	
	}

private:
	GLuint dlist_;
};
typedef std::shared_ptr<fullscreen_quad> fullscreen_quad_ptr;


class texture
{
public:
	texture(size_t width, size_t height) : width_(width), height_(height)
	{	
		glGenTextures(1, &texture_);	
		
		glBindTexture(GL_TEXTURE_2D, texture_);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}

	~texture()
	{		
		glDeleteTextures(1, &texture_);
	}
	
	size_t width() const { return width_; }
	size_t height() const { return height_; }
	GLuint handle() { return texture_; }

private:
	GLuint texture_;
	const size_t width_;
	const size_t height_;
};
typedef std::shared_ptr<texture> texture_ptr;

class pixel_buffer
{
public:

	pixel_buffer(size_t width, size_t height) : size_(width*height*4), texture_(width, height)
	{
		glGenBuffersARB(1, &pbo_);
	}

	~pixel_buffer()
	{
		glDeleteBuffers(1, &pbo_);
	}

	void begin_write(void* src)
	{
		glBindTexture(GL_TEXTURE_2D, texture_.handle());
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size_, 0, GL_STREAM_DRAW);
		void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		assert(ptr); 
		common::image::copy(ptr, src, size_);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	void end_write()
	{
		glBindTexture(GL_TEXTURE_2D, texture_.handle());
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_.width(), texture_.height(), GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}

	void begin_read(GLuint buffer)
	{
		glReadBuffer(buffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo_);
		glBufferData(GL_PIXEL_PACK_BUFFER_ARB, size_, NULL, GL_STREAM_READ);
		glReadPixels(0, 0, texture_.width(), texture_.height(), GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}

	void end_read(void* dest)
	{
		void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);   
		common::image::copy(dest, ptr, size_);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}
		
	size_t size() const { return size_; }
	GLuint pbo_handle() { return pbo_; }
	GLuint texture_handle() { return texture_.handle(); }

private:
	texture texture_;
	GLuint pbo_;
	size_t size_;
};
typedef std::shared_ptr<pixel_buffer> pixel_buffer_ptr;

class frame_buffer
{
public:
	frame_buffer(size_t width, size_t height) : pbo_(width, height), width_(width), height_(height)
	{
		glGenFramebuffersEXT(1, &fbo_);
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
		glBindTexture(GL_TEXTURE_2D, pbo_.texture_handle());
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pbo_.texture_handle(), 0);
	}

	~frame_buffer()
	{
		glDeleteFramebuffersEXT(1, &fbo_);
	}
	
	void begin_read()
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
		pbo_.begin_read(GL_COLOR_ATTACHMENT0_EXT);
	}

	void end_read(void* dest)
	{
		pbo_.end_read(dest);		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	GLuint handle() { return fbo_; }
	
private:
	pixel_buffer pbo_;
	GLuint fbo_;
	size_t size_;
	size_t width_;
	size_t height_;
};
typedef std::shared_ptr<frame_buffer> frame_buffer_ptr;

struct frame_processor::implementation
{	
	implementation(const frame_format_desc& format_desc) : format_desc_(format_desc)
	{
		queue_.set_capacity(5);
		thread_ = boost::thread([=]{run();});
		empty_frame_ = clear_frame(std::make_shared<system_frame>(format_desc_.size));

		// triple buffer, add two frames to offset returning frames
		finished_frames_.push(empty_frame_);
		finished_frames_.push(empty_frame_);
	}

	~implementation()
	{
		queue_.push(nullptr);
		finished_frames_.push(nullptr);
		thread_.join();
	}
		
	void composite(const std::vector<frame_ptr>& frames)
	{
		if(frames.empty())
			finished_frames_.push(empty_frame_);
		else
			queue_.push([=]{do_composite(frames);});
	}

	void do_composite(const std::vector<frame_ptr>& frames)
	{					
		// END PREVIOUS READ
		if(reading_fbo_)
		{
			reading_fbo_->end_read(reading_result_frame_->data());
			finished_frames_.push(reading_result_frame_);

			reading_fbo_ = nullptr;
			reading_result_frame_ = nullptr;
		}

		// END PREVIOUS WRITE
		frame_buffer_ptr written_fbo;
		if(!writing_pbo_group_.empty())
		{
			// END
			written_fbo = get_fbo();

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, written_fbo->handle());
			glClear(GL_COLOR_BUFFER_BIT);	

			for(size_t n = 0; n < writing_pbo_group_.size(); ++n)
			{
				writing_pbo_group_[n]->end_write();
				glBindTexture(GL_TEXTURE_2D, writing_pbo_group_[n]->texture_handle());
				quad_->draw();
			}
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);		
			writing_pbo_group_.clear();
		}
		
		// BEGIN NEW READ		
		if(written_fbo)
		{	
			written_fbo->begin_read();
			reading_fbo_ = written_fbo; 
			reading_result_frame_ = writing_result_frame_;

			writing_result_frame_ = nullptr;
		}

		// BEGIN NEW WRITE
		if(!frames.empty())
		{
			for(size_t n = 0; n < frames.size(); ++n)
			{
				auto pbo = get_pbo();
				pbo->begin_write(frames[n]->data());

				writing_pbo_group_.push_back(pbo);
			}
			
			writing_result_frame_ = std::make_shared<system_frame>(format_desc_.size);
			boost::range::for_each(frames, std::bind(&copy_frame_audio<frame_ptr>, writing_result_frame_, std::placeholders::_1)); 
		}
	}

	frame_ptr get_frame()
	{
		frame_ptr frame;
		finished_frames_.pop(frame);
		return frame;
	}
		
	void init()	
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);			
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glViewport(0, 0, format_desc_.width, format_desc_.height);
		glLoadIdentity();

		quad_.reset(new fullscreen_quad());
	}

	void run()
	{
		init();

		std::function<void()> func;
		queue_.pop(func);

		while(func != nullptr)
		{
			func();
			queue_.pop(func);
		}
	}

	pixel_buffer_ptr get_pbo()
	{
		if(pbo_pool_.empty())
			pbo_pool_.push_back(std::make_shared<pixel_buffer>(format_desc_.width, format_desc_.height));

		auto pbo = pbo_pool_.front();
		pbo_pool_.pop_front();

		return pixel_buffer_ptr(pbo.get(), [=](pixel_buffer*){pbo_pool_.push_back(pbo);});
	}

	frame_buffer_ptr get_fbo()
	{
		if(fbo_pool_.empty())
			fbo_pool_.push_back(std::make_shared<frame_buffer>(format_desc_.width, format_desc_.height));

		auto fbo = fbo_pool_.front();
		fbo_pool_.pop_front();

		return frame_buffer_ptr(fbo.get(), [=](frame_buffer*){fbo_pool_.push_back(fbo);});
	}

	std::vector<pixel_buffer_ptr>			  writing_pbo_group_;
	frame_ptr								  writing_result_frame_;

	frame_buffer_ptr						  reading_fbo_;
	frame_ptr								  reading_result_frame_;

	std::deque<pixel_buffer_ptr> pbo_pool_;
	std::deque<frame_buffer_ptr> fbo_pool_;
	
	std::unique_ptr<sf::Context> context_;
	fullscreen_quad_ptr quad_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<std::function<void()>> queue_;
	tbb::concurrent_bounded_queue<frame_ptr> finished_frames_;

	frame_format_desc format_desc_;
	frame_ptr empty_frame_;
};
	
frame_processor::frame_processor(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_processor::composite(const std::vector<frame_ptr>& frames){ impl_->composite(frames);}
frame_ptr frame_processor::get_frame(){ return impl_->get_frame();}

}}