#include "../../StdAfx.h"

#include "write_buffer.h"

#include <common/exception/exceptions.h>
#include <common/gl/utility.h>

namespace caspar { namespace core {

GLenum FORMAT[] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR, GL_BGRA};
GLenum INTERNAL_FORMAT[] = {0, GL_LUMINANCE8, GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8};			

struct write_buffer::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height, size_t depth) : width_(width), height_(height), size_(width*height*depth), pbo_(0), format_(FORMAT[depth]), data_(nullptr), texture_(0)
	{
		GL(glGenBuffers(1, &pbo_));
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_));
		GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
		
		if(!pbo_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to allocate buffer."));

		GL(glGenTextures(1, &texture_));
		GL(glBindTexture(GL_TEXTURE_2D, texture_));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_FORMAT[depth], width_, height_, 0, format_, GL_UNSIGNED_BYTE, NULL));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
				
		if(!texture_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to allocate texture."));

		CASPAR_LOG(trace) << "[write_buffer] Allocated.";
	}

	~implementation()
	{
		glDeleteBuffers(1, &pbo_);
		glDeleteTextures(1, &texture_);
	}	
		
	void unmap()
	{
		if(!data_)
			return;

		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_));

		GL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
		data_ = nullptr;

		GL(glBindTexture(GL_TEXTURE_2D, texture_));
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format_, GL_UNSIGNED_BYTE, NULL));

		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	}

	void map()
	{
		if(data_)
			return;
		
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_));

		GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
		data_= glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

		if(!data_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("glMapBuffer failed"));
	}
				
	GLuint pbo_;
	GLuint texture_;

	const size_t width_;
	const size_t height_;
	const size_t size_;
	
	GLenum format_;
	void* data_;
};

write_buffer::write_buffer(write_buffer&& other) : impl_(std::move(other.impl_)){}
write_buffer::write_buffer(size_t width, size_t height, size_t depth) : impl_(new implementation(width, height, depth)){}
write_buffer& write_buffer::operator=(write_buffer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void* write_buffer::data(){return impl_->data_;}
void write_buffer::unmap() { impl_->unmap();}
void write_buffer::map() {impl_->map();} 
size_t write_buffer::size() const {return impl_->size_;}
size_t write_buffer::width() const {return impl_->width_;}
size_t write_buffer::height() const {return impl_->height_;}
}}