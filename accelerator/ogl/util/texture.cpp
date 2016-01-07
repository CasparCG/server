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

#include "../../StdAfx.h"

#include "texture.h"

#include "buffer.h"

#include <common/except.h>
#include <common/gl/gl_check.h>

#include <GL/glew.h>

#include <tbb/atomic.h>

#include <boost/thread/future.hpp>

namespace caspar { namespace accelerator { namespace ogl {
	
static GLenum FORMAT[] = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};	
static GLenum TYPE[]			= { 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE };
static GLenum READPIXELS_TYPE[]	= { 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_8_8_8_8_REV };

static tbb::atomic<int>			g_total_count;
static tbb::atomic<std::size_t>	g_total_size;

struct texture::impl : boost::noncopyable
{
	GLuint	id_;

	const int	width_;
	const int	height_;
	const int	stride_;
	const bool	mipmapped_;
public:
	impl(int width, int height, int stride, bool mipmapped) 
		: width_(width)
		, height_(height)
		, stride_(stride)
		, mipmapped_(mipmapped)
	{	
		CASPAR_LOG_CALL(trace) << "texture::texture() <- " << get_context();

		GL(glGenTextures(1, &id_));
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR)));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[stride_], width_, height_, 0, FORMAT[stride_], TYPE[stride_], NULL));

		if (mipmapped)
		{
			enable_anosotropic_filtering_if_available();
			GL(glGenerateMipmap(GL_TEXTURE_2D));
		}

		GL(glBindTexture(GL_TEXTURE_2D, 0));
		g_total_count++;
		g_total_size += static_cast<std::size_t>(width * height * stride * (mipmapped ? 1.33 : 1.0));
		//CASPAR_LOG(trace) << "[texture] [" << ++g_total_count << L"] allocated size:" << width*height*stride;	
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

	~impl()
	{
		CASPAR_LOG_CALL(trace) << "texture::~texture() <- " << get_context();
		glDeleteTextures(1, &id_);
		g_total_size -= static_cast<std::size_t>(width_ * height_ * stride_ * (mipmapped_ ? 1.33 : 1.0));
		g_total_count--;
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
		CASPAR_LOG_CALL(trace) << "texture::copy_from(buffer&) <- " << get_context();
		source.unmap();
		source.bind();
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], NULL));

		if (mipmapped_)
			GL(glGenerateMipmap(GL_TEXTURE_2D));

		GL(glBindTexture(GL_TEXTURE_2D, 0));
		source.unbind();
		source.map(); // Just map it back since map will orphan buffer.
	}

	void copy_to(buffer& dest)
	{
		CASPAR_LOG_CALL(trace) << "texture::copy_to(buffer&) <- " << get_context();
		dest.unmap();
		dest.bind();
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0));
		GL(glReadPixels(0, 0, width_, height_, FORMAT[stride_], READPIXELS_TYPE[stride_], NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		dest.unbind();
		GL(glFlush());
	}
};

texture::texture(int width, int height, int stride, bool mipmapped) : impl_(new impl(width, height, stride, mipmapped)){}
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
bool texture::mipmapped() const { return impl_->mipmapped_; }
std::size_t texture::size() const { return static_cast<std::size_t>(impl_->width_*impl_->height_*impl_->stride_); }
int texture::id() const{ return impl_->id_;}

boost::property_tree::wptree texture::info()
{
	boost::property_tree::wptree info;

	info.add(L"total_count", g_total_count);
	info.add(L"total_size", g_total_size);

	return info;
}

}}}
