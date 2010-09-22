#include "../stdafx.h"

#include "pixel_buffer.h"

namespace caspar { namespace common { namespace gpu {
	
write_pixel_buffer::write_pixel_buffer(size_t width, size_t height) : size_(width*height*4), texture_(width, height), written_(false)
{
	CASPAR_GL_CHECK(glGenBuffersARB(1, &pbo_));
}

write_pixel_buffer::~write_pixel_buffer()
{
	glDeleteBuffers(1, &pbo_);
}

void write_pixel_buffer::begin_write(void* src)
{
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_.handle()));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size_, NULL, GL_STREAM_DRAW));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY));
	memcpy(ptr, src, size_);
	CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
	written_ = true;
}

void write_pixel_buffer::end_write()
{
	if(!written_)
		return;
	
	written_ = false;
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_.handle()));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_.width(), texture_.height(), GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL));
}

void write_pixel_buffer::draw()
{
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_.handle()));
	quad_.draw();
}
	
size_t write_pixel_buffer::size() const { return size_; }
size_t write_pixel_buffer::width() const { return texture_.width(); }
size_t write_pixel_buffer::height() const { return texture_.height(); }
GLuint write_pixel_buffer::pbo_handle() { return pbo_; }

read_pixel_buffer::read_pixel_buffer(size_t width, size_t height) : size_(width*height*4), width_(width), height_(height), reading_(false)
{
	CASPAR_GL_CHECK(glGenBuffersARB(1, &pbo_));
}

read_pixel_buffer::~read_pixel_buffer()
{
	glDeleteBuffers(1, &pbo_);
}
	
void read_pixel_buffer::begin_read()
{
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glBufferData(GL_PIXEL_PACK_BUFFER_ARB, size_, NULL, GL_STREAM_READ));
	CASPAR_GL_CHECK(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	reading_ = true;
}

void read_pixel_buffer::end_read(void* dest)
{
	if(!reading_)
		return;
	reading_ = false;

	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo_));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY));   
	memcpy(dest, ptr, size_);
	CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL));
}
		
size_t read_pixel_buffer::size() const { return size_; }
size_t read_pixel_buffer::width() const { return width_; }
size_t read_pixel_buffer::height() const { return height_; }
GLuint read_pixel_buffer::pbo_handle() { return pbo_; }


}}}