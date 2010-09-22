#include "../stdafx.h"

#include "pixel_buffer.h"

#include "../image/copy.h"

namespace caspar { namespace common { namespace gpu {
	
pixel_buffer::pixel_buffer(size_t width, size_t height) : size_(width*height*4), written_(false), reading_(false), width_(width), height_(height)
{
	CASPAR_GL_CHECK(glGenBuffersARB(1, &pbo_));
}

pixel_buffer::~pixel_buffer()
{
	glDeleteBuffers(1, &pbo_);
}

void pixel_buffer::write_to_pbo(void* src)
{
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size_, NULL, GL_STREAM_DRAW));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY));
	image::copy_SSE2(ptr, src, size_);
	CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
	written_ = true;
}

void pixel_buffer::write_to_texture(texture& texture)
{
	if(!written_)
		return;
	
	written_ = false;
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture.handle()));
	CASPAR_GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width(), texture.height(), GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL));
}
		
void pixel_buffer::read_to_pbo()
{
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo_));
	CASPAR_GL_CHECK(glBufferData(GL_PIXEL_PACK_BUFFER_ARB, size_, NULL, GL_STREAM_READ));
	CASPAR_GL_CHECK(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	reading_ = true;
}

void pixel_buffer::read_to_memory(void* dest)
{
	if(!reading_)
		return;
	reading_ = false;

	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo_));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY));   
	image::copy_SSE2(dest, ptr, size_);
	CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL));
}
		
size_t pixel_buffer::size() const { return size_; }
size_t pixel_buffer::width() const { return width_; }
size_t pixel_buffer::height() const { return height_; }
GLuint pixel_buffer::pbo_handle() { return pbo_; }


}}}