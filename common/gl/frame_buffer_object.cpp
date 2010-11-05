#include "../StdAfx.h"

#include "frame_buffer_object.h"

#include "../../common/gl/gl_check.h"

#include <Glee.h>

#include <memory>

namespace caspar { namespace common { namespace gl {

struct frame_buffer_object::implementation
{
public:
	implementation(size_t width, size_t height, GLenum mode) : mode_(mode)
	{
		GL(glGenTextures(1, &texture_));	
		GL(glBindTexture(GL_TEXTURE_2D, texture_));			
		GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, 
							GL_UNSIGNED_BYTE, NULL));
		GL(glGenFramebuffersEXT(1, &fbo_));		
		GL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_));
		GL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, mode_, GL_TEXTURE_2D, 
										texture_, 0));
	}
	
	~implementation()
	{
		glDeleteFramebuffersEXT(1, &fbo_);
		glDeleteTextures(1, &texture_);
	}

	void bind_pixel_source()
	{
		GL(glReadBuffer(mode_));
	}

	GLuint texture_;
	GLuint fbo_;
	GLenum mode_;
	size_t width_;
	size_t height_;
};

frame_buffer_object::frame_buffer_object(){}
frame_buffer_object::frame_buffer_object(size_t width, size_t height, GLenum mode)
	: impl_(new implementation(width, height, mode)){}
void frame_buffer_object::create(size_t width, size_t height, GLenum mode)
{
	impl_.reset(new implementation(width, height, mode));
}
void frame_buffer_object::bind_pixel_source() {impl_->bind_pixel_source();}
}}}