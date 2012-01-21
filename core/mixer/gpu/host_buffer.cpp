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

#include "host_buffer.h"

#include "device_buffer.h"
#include "ogl_device.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <gl/glew.h>

#include <tbb/atomic.h>

namespace caspar { namespace core {

static tbb::atomic<int> g_w_total_count;
static tbb::atomic<int> g_r_total_count;
																																								
struct host_buffer::impl : boost::noncopyable
{	
	GLuint		pbo_;
	const int	size_;
	void*		data_;
	GLenum		usage_;
	GLenum		target_;

public:
	impl(int size, usage_t usage) 
		: size_(size)
		, data_(nullptr)
		, pbo_(0)
		, target_(usage == write_only ? GL_PIXEL_UNPACK_BUFFER : GL_PIXEL_PACK_BUFFER)
		, usage_(usage == write_only ? GL_STREAM_DRAW : GL_STREAM_READ)
	{
		GL(glGenBuffers(1, &pbo_));
		GL(glBindBuffer(target_, pbo_));
		if(usage_ != write_only)	
			GL(glBufferData(target_, size_, NULL, usage_));	
		GL(glBindBuffer(target_, 0));

		if(!pbo_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to allocate buffer."));

		CASPAR_LOG(trace) << "[host_buffer] [" << ++(usage_ == write_only ? g_w_total_count : g_r_total_count) << L"] allocated size:" << size_ << " usage: " << (usage == write_only ? "write_only" : "read_only");
	}	

	~impl()
	{
		try
		{
			GL(glDeleteBuffers(1, &pbo_));
			//CASPAR_LOG(trace) << "[host_buffer] [" << --(usage_ == write_only ? g_w_total_count : g_r_total_count) << L"] deallocated size:" << size_ << " usage: " << (usage_ == write_only ? "write_only" : "read_only");
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	void map()
	{
		if(data_)
			return;

		if(usage_ == write_only)			
			GL(glBufferData(target_, size_, NULL, usage_));	// Notify OpenGL that we don't care about previous data.
		
		GL(glBindBuffer(target_, pbo_));
		data_ = GL2(glMapBuffer(target_, usage_ == GL_STREAM_DRAW ? GL_WRITE_ONLY : GL_READ_ONLY));  
		GL(glBindBuffer(target_, 0)); 
		if(!data_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to map target_ OpenGL Pixel Buffer Object."));
	}
	
	void unmap()
	{
		if(!data_)
			return;
		
		if(usage_ == read_only)			
			GL(glBufferData(target_, size_, NULL, usage_));	// Notify OpenGL that we don't care about previous data.

		GL(glBindBuffer(target_, pbo_));
		GL(glUnmapBuffer(target_));	
		data_ = nullptr;		
		GL(glBindBuffer(target_, 0));
	}

	void bind()
	{
		GL(glBindBuffer(target_, pbo_));
	}

	void unbind()
	{
		GL(glBindBuffer(target_, 0));
	}

	void begin_read(int width, int height, GLuint format)
	{
		unmap();
		bind();
		GL(glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, NULL));
		unbind();
	}
};

host_buffer::host_buffer(int size, usage_t usage) : impl_(new impl(size, usage)){}
const void* host_buffer::data() const {return impl_->data_;}
void* host_buffer::data() {return impl_->data_;}
void host_buffer::map(){impl_->map();}
void host_buffer::unmap(){impl_->unmap();}
void host_buffer::bind(){impl_->bind();}
void host_buffer::unbind(){impl_->unbind();}
void host_buffer::begin_read(int width, int height, GLuint format){impl_->begin_read(width, height, format);}
int host_buffer::size() const { return impl_->size_; }

}}