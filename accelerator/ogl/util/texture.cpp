/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../stdafx.h"

#include "texture.h"

#include "buffer.h"

#include <common/except.h>
#include <common/gl/gl_check.h>

#include <gl/glew.h>

#include <tbb/atomic.h>

#include <boost/thread/future.hpp>

namespace caspar { namespace accelerator { namespace ogl {
	
static GLenum FORMAT[] = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};	
static GLenum TYPE[] = {0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_8_8_8_8_REV};	

static tbb::atomic<int> g_total_count;

struct texture::impl : boost::noncopyable
{
	GLuint	id_;

	const int width_;
	const int height_;
	const int stride_;
public:
	impl(int width, int height, int stride) 
		: width_(width)
		, height_(height)
		, stride_(stride)
	{	
		GL(glGenTextures(1, &id_));
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[stride_], width_, height_, 0, FORMAT[stride_], TYPE[stride_], NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		//CASPAR_LOG(trace) << "[texture] [" << ++g_total_count << L"] allocated size:" << width*height*stride;	
	}	

	~impl()
	{
		glDeleteTextures(1, &id_);
	}
	
	void bind()
	{
		GL(glBindTexture(GL_TEXTURE_2D, id_));
	}

	void bind(int index)
	{
		GL(glActiveTexture(GL_TEXTURE0+index));
		bind();
	}

	void unbind()
	{
		GL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	void attach()
	{		
		GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, id_, 0));
	}

	void clear()
	{
		attach();		
		GL(glClear(GL_COLOR_BUFFER_BIT));
	}
		
	void copy_from(buffer& source)
	{
		source.unmap();
		source.bind();
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		source.unbind();
		source.map(); // Just map it back since map will orphan buffer.
	}

	void copy_to(buffer& dest)
	{
		dest.unmap();
		dest.bind();
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0));
		GL(glReadPixels(0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		dest.unbind();
		GL(glFlush());
	}
};

texture::texture(int width, int height, int stride) : impl_(new impl(width, height, stride)){}
texture::texture(texture&& other) : impl_(std::move(other.impl_)){}
texture::~texture(){}
texture& texture::operator=(texture&& other){impl_ = std::move(other.impl_); return *this;}
void texture::bind(int index){impl_->bind(index);}
void texture::unbind(){impl_->unbind();}
void texture::attach(){impl_->attach();}
void texture::clear(){impl_->clear();}
void texture::copy_from(buffer& source){impl_->copy_from(source);}
void texture::copy_to(buffer& dest){impl_->copy_to(dest);}
int texture::width() const { return impl_->width_; }
int texture::height() const { return impl_->height_; }
int texture::stride() const { return impl_->stride_; }
std::size_t texture::size() const { return static_cast<std::size_t>(impl_->width_*impl_->height_*impl_->stride_); }
int texture::id() const{ return impl_->id_;}

}}}