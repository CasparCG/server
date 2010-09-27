#include "../../StdAfx.h"

#include "frame.h"
#include "../algorithm.h"
#include "../../../common/image/copy.h"

namespace caspar { namespace gpu {
	
gpu_frame::gpu_frame(size_t width, size_t height, void* tag) 
	: pbo_(0), data_(nullptr), tag_(tag), width_(width), height_(height), size_(width*height*4), reading_(false), texture_(0)
{	
	CASPAR_GL_CHECK(glGenBuffers(1, &pbo_));
}

gpu_frame::~gpu_frame()
{
	if(pbo_ != 0)
		glDeleteBuffers(1, &pbo_);
	if(texture_ != 0)
		glDeleteTextures(1, &texture_);
}

void gpu_frame::write_lock()
{
	if(texture_ == 0)
	{
		CASPAR_GL_CHECK(glGenTextures(1, &texture_));

		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));

		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		CASPAR_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	}

	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_));
	if(data_ != nullptr)
	{
		CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
		data_ = nullptr;
	}
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
	CASPAR_GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
}

bool gpu_frame::write_unlock()
{
	if(data_ != nullptr)
		return false;
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_));
	CASPAR_GL_CHECK(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	data_ = reinterpret_cast<unsigned char*>(ptr);
	if(!data_)
		BOOST_THROW_EXCEPTION(std::bad_alloc());
	return true;
}
	
void gpu_frame::read_lock(GLenum mode)
{	
	CASPAR_GL_CHECK(glReadBuffer(mode));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_));
	if(data_ != nullptr)	
	{	
		CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));	
		data_ = nullptr;
	}
	CASPAR_GL_CHECK(glBufferData(GL_PIXEL_PACK_BUFFER, size_, NULL, GL_STREAM_READ));	
	CASPAR_GL_CHECK(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
	reading_ = true;
}

bool gpu_frame::read_unlock()
{
	if(data_ != nullptr || !reading_)
		return false;
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_));
	void* ptr = CASPAR_GL_CHECK(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));   
	CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
	data_ = reinterpret_cast<unsigned char*>(ptr);
	if(!data_)
		BOOST_THROW_EXCEPTION(std::bad_alloc());
	reading_ = false;
	return true;
}

void gpu_frame::draw()
{
	CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
	glEnd();
}

void* gpu_frame::tag() const
{
	return tag_;
}
	
unsigned char* gpu_frame::data()
{
	if(data_ == nullptr)
		BOOST_THROW_EXCEPTION(invalid_operation());
	return data_;
}
size_t gpu_frame::size() const { return size_; }
size_t gpu_frame::width() const { return width_;}
size_t gpu_frame::height() const { return height_;}

}}