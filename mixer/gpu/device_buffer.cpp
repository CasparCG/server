/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../stdafx.h"

#include "device_buffer.h"

#include <common/gl/gl_check.h>
namespace caspar { namespace mixer {
	
GLenum FORMAT[] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR, GL_BGRA};
GLenum INTERNAL_FORMAT[] = {0, GL_LUMINANCE8, GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8};	

struct device_buffer::implementation : boost::noncopyable
{
	GLuint id_;

	const size_t width_;
	const size_t height_;
	const size_t stride_;

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
		//CASPAR_LOG(trace) << "[device_buffer] allocated size:" << width*height*stride;	
	}	

	~implementation()
	{
		GL(glDeleteTextures(1, &id_));
	}
	
	void bind()
	{
		GL(glBindTexture(GL_TEXTURE_2D, id_));
	}

	void unbind()
	{
		GL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	void read(host_buffer& source)
	{
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		source.unmap();
		source.bind();
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));
		source.unbind();
		GL(glBindTexture(GL_TEXTURE_2D, 0));
	}
	
	void write(host_buffer& target)
	{
		GL(glBindTexture(GL_TEXTURE_2D, id_));
		target.unmap();
		target.bind();
		GL(glReadPixels(0, 0, width_, height_, FORMAT[stride_], GL_UNSIGNED_BYTE, NULL));
		target.unbind();
		GL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	void attach(int index)
	{
		GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, id_, 0));
	}
};

device_buffer::device_buffer(size_t width, size_t height, size_t stride) : impl_(new implementation(width, height, stride)){}
size_t device_buffer::stride() const { return impl_->stride_; }
size_t device_buffer::width() const { return impl_->width_; }
size_t device_buffer::height() const { return impl_->height_; }
void device_buffer::attach(int index){impl_->attach(index);}
void device_buffer::bind(){impl_->bind();}
void device_buffer::unbind(){impl_->unbind();}
void device_buffer::read(host_buffer& source){impl_->read(source);}
void device_buffer::write(host_buffer& target){impl_->write(target);}

}}