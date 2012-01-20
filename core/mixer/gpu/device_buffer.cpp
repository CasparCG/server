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

unsigned int format(int stride)
{
	return FORMAT[stride];
}

static tbb::atomic<int> g_total_count;

struct device_buffer::impl : boost::noncopyable
{
	GLuint id_;

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
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[stride_], width_, height_, 0, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));
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
};

device_buffer::device_buffer(int width, int height, int stride) : impl_(new impl(width, height, stride)){}
int device_buffer::stride() const { return impl_->stride_; }
int device_buffer::width() const { return impl_->width_; }
int device_buffer::height() const { return impl_->height_; }
int device_buffer::id() const{ return impl_->id_;}


}}