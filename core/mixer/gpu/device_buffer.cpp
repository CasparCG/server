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

#include "device_buffer.h"

#include "fence.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <gl/glew.h>

#include <tbb/atomic.h>

namespace caspar { namespace core {
	
static GLenum FORMAT[] = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};	

unsigned int format(size_t stride)
{
	return FORMAT[stride];
}

static tbb::atomic<int> g_total_count;

struct device_buffer::implementation : boost::noncopyable
{
	GLuint id_;

	const size_t width_;
	const size_t height_;
	const size_t stride_;

	fence		 fence_;

public:
	implementation(size_t width, size_t height, size_t stride) 
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
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[stride_], width_, height_, 0, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		CASPAR_LOG(trace) << "[device_buffer] [" << ++g_total_count << L"] allocated size:" << width*height*stride;	
	}	

	~implementation()
	{
		try
		{
			GL(glDeleteTextures(1, &id_));
			//CASPAR_LOG(trace) << "[device_buffer] [" << --g_total_count << L"] deallocated size:" << width_*height_*stride_;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
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

	void begin_read()
	{
		bind();
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));
		unbind();
		fence_.set();
	}
	
	bool ready() const
	{
		return fence_.ready();
	}
};

device_buffer::device_buffer(size_t width, size_t height, size_t stride) : impl_(new implementation(width, height, stride)){}
size_t device_buffer::stride() const { return impl_->stride_; }
size_t device_buffer::width() const { return impl_->width_; }
size_t device_buffer::height() const { return impl_->height_; }
void device_buffer::bind(int index){impl_->bind(index);}
void device_buffer::unbind(){impl_->unbind();}
void device_buffer::begin_read(){impl_->begin_read();}
bool device_buffer::ready() const{return impl_->ready();}
int device_buffer::id() const{ return impl_->id_;}


}}