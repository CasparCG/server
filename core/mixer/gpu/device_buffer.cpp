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
#include "accelerator.h"

#include <common/except.h>
#include <common/gl/gl_check.h>

#include <gl/glew.h>

#include <tbb/atomic.h>

#include <boost/thread/future.hpp>

namespace caspar { namespace core { namespace gpu {
	
static GLenum FORMAT[] = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};	
static GLenum TYPE[] = {0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_8_8_8_8_REV};	

unsigned int format(int stride)
{
	return FORMAT[stride];
}

static tbb::atomic<int> g_total_count;

struct device_buffer::impl : boost::noncopyable
{
	std::weak_ptr<accelerator>	parent_;
	GLuint						id_;

	const int width_;
	const int height_;
	const int stride_;
public:
	impl(std::weak_ptr<accelerator> parent, int width, int height, int stride) 
		: parent_(parent)
		, width_(width)
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
		CASPAR_LOG(trace) << "[device_buffer] [" << ++g_total_count << L"] allocated size:" << width*height*stride;	
	}	

	~impl()
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
		
	void copy_from(const spl::shared_ptr<host_buffer>& source)
	{
		auto ogl = parent_.lock();
		if(!ogl)
			BOOST_THROW_EXCEPTION(invalid_operation());

		ogl->begin_invoke([=]
		{
			source->unmap();
			source->bind();
			GL(glBindTexture(GL_TEXTURE_2D, id_));
			GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], NULL));
			GL(glBindTexture(GL_TEXTURE_2D, 0));
			source->unbind();
		}, task_priority::high_priority);
	}

	void copy_to(const spl::shared_ptr<host_buffer>& dest)
	{
		auto ogl = parent_.lock();
		if(!ogl)
			BOOST_THROW_EXCEPTION(invalid_operation());

		ogl->begin_invoke([=]
		{
			dest->unmap();
			dest->bind();
			GL(glBindTexture(GL_TEXTURE_2D, id_));
			GL(glReadBuffer(GL_COLOR_ATTACHMENT0));
			GL(glReadPixels(0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], NULL));
			GL(glBindTexture(GL_TEXTURE_2D, 0));
			dest->unbind();
			GL(glFlush());
		}, task_priority::high_priority);
	}
};

device_buffer::device_buffer(std::weak_ptr<accelerator> parent, int width, int height, int stride) : impl_(new impl(parent, width, height, stride)){}
int device_buffer::stride() const { return impl_->stride_; }
int device_buffer::width() const { return impl_->width_; }
int device_buffer::height() const { return impl_->height_; }
void device_buffer::bind(int index){impl_->bind(index);}
void device_buffer::unbind(){impl_->unbind();}
void device_buffer::copy_from(const spl::shared_ptr<host_buffer>& source){impl_->copy_from(source);}
void device_buffer::copy_to(const spl::shared_ptr<host_buffer>& dest){impl_->copy_to(dest);}
int device_buffer::id() const{ return impl_->id_;}

}}}