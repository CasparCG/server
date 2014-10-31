/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace core {
	
static GLenum FORMAT[] = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};	

unsigned int format(size_t stride)
{
	return FORMAT[stride];
}

static tbb::atomic<int> g_instance_id;
static tbb::atomic<int> g_total_count;
static tbb::atomic<int> g_total_size;

struct device_buffer::implementation : boost::noncopyable
{
	GLuint			id_;
	int				instance_id_;

	const size_t	width_;
	const size_t	height_;
	const size_t	stride_;
	const size_t	size_;
	const bool		mipmapped_;

	fence			fence_;

public:
	implementation(size_t width, size_t height, size_t stride, bool mipmapped) 
		: instance_id_(++instance_id_)
		, width_(width)
		, height_(height)
		, stride_(stride)
		, size_(static_cast<size_t>(width * height * stride * (mipmapped ? 1.33 : 1.0)))
		, mipmapped_(mipmapped)
	{	
		GL(glGenTextures(1, &id_));
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR)));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[stride_], width_, height_, 0, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));

		if (mipmapped)
		{
			enable_anosotropic_filtering_if_available();
			GL(glGenerateMipmap(GL_TEXTURE_2D));
		}

		GL(glBindTexture(GL_TEXTURE_2D, 0));
		g_total_size += size_;
		++g_total_count;
		CASPAR_LOG(trace) << "[device_buffer] [" << instance_id_ << L"] allocated size:" << size_ << " for a total of: " << g_total_size;
	}

	void enable_anosotropic_filtering_if_available()
	{
		static auto AVAILABLE = glewIsSupported("GL_EXT_texture_filter_anisotropic");

		if (!AVAILABLE)
			return;

		static GLfloat MAX_ANISOTROPY = []() -> GLfloat
		{
			GLfloat anisotropy;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
			return anisotropy;
		}();

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MAX_ANISOTROPY);
	}

	~implementation()
	{
		try
		{
			GL(glDeleteTextures(1, &id_));
			g_total_size -= size_;
			--g_total_count;
			CASPAR_LOG(trace) << "[device_buffer] [" << instance_id_ << L"] deallocated size:" << size_ << " for a remaining total of: " << g_total_size;
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

		if (mipmapped_)
			GL(glGenerateMipmap(GL_TEXTURE_2D));

		unbind();
		fence_.set();
	}
	
	bool ready() const
	{
		return fence_.ready();
	}
};

device_buffer::device_buffer(size_t width, size_t height, size_t stride, bool mipmapped) : impl_(new implementation(width, height, stride, mipmapped)){}
size_t device_buffer::stride() const { return impl_->stride_; }
size_t device_buffer::width() const { return impl_->width_; }
size_t device_buffer::height() const { return impl_->height_; }
size_t device_buffer::size() const { return impl_->size_; }
void device_buffer::bind(int index){impl_->bind(index);}
void device_buffer::unbind(){impl_->unbind();}
void device_buffer::begin_read(){impl_->begin_read();}
bool device_buffer::ready() const{return impl_->ready();}
int device_buffer::id() const{ return impl_->id_;}

boost::property_tree::wptree device_buffer::info()
{
	boost::property_tree::wptree info;

	info.add(L"total_count", g_total_count);
	info.add(L"total_size", g_total_size);

	return info;
}

}}
