#include "../../StdAfx.h"

#include "read_buffer.h"

#include <common/gl/utility.h>

namespace caspar { namespace core {
																																							
struct read_buffer::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) : width_(width), height_(height), size_(width*height*4), data_(nullptr), pbo_(0)
	{
		GL(glGenBuffers(1, &pbo_));
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_));
		GL(glBufferData(GL_PIXEL_PACK_BUFFER, size_, NULL, GL_STREAM_READ));	
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));

		if(!pbo_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to allocate buffer."));
		CASPAR_LOG(trace) << "[read_buffer] Allocated.";
	}	

	~implementation()
	{
		glDeleteBuffers(1, &pbo_);
	}

	void map()
	{
		if(data_)
			return;

		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_));
		data_ = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);   
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
		if(!data_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to map GL_PIXEL_PACK_BUFFER OpenGL Pixel Buffer Object."));
	}

	void unmap()
	{
		if(!data_)
			return;

		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_));
		GL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));	
		data_ = nullptr;
		GL(glBufferData(GL_PIXEL_PACK_BUFFER, size_, NULL, GL_STREAM_READ));	
		GL(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
	}
	
	GLuint pbo_;

	const size_t size_;
	const size_t width_;
	const size_t height_;

	void* data_;
	std::vector<short> audio_data_;
};

read_buffer::read_buffer(size_t width, size_t height) : impl_(new implementation(width, height)){}
const void* read_buffer::data() const {return impl_->data_;}
void read_buffer::map(){impl_->map();}
void read_buffer::unmap(){impl_->unmap();}
size_t read_buffer::size() const { return impl_->size_; }
size_t read_buffer::width() const { return impl_->width_; }
size_t read_buffer::height() const { return impl_->height_; }

}}