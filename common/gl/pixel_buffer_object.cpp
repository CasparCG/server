#include "../StdAfx.h"

#include "pixel_buffer_object.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/gl_check.h"
#include "../../common/utility/memory.h"

namespace caspar { namespace common { namespace gl {
																																							
struct pixel_buffer_object::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height, GLenum format) 
		: width_(width), height_(height), pbo_(0), format_(format),
			texture_(0), writing_(false), reading_(false), mapped_(false)
	{
		switch(format)
		{
		case GL_RGBA:
		case GL_BGRA:
			internal_ = GL_RGBA8;
			size_ = width*height*4;
			break;
		case GL_BGR:
			internal_ = GL_RGB8;
			size_ = width*height*3;
			break;
		case GL_LUMINANCE_ALPHA:
			internal_ = GL_LUMINANCE_ALPHA;
			size_ = width*height*2;
			break;
		case GL_LUMINANCE:
		case GL_ALPHA:
			internal_ = GL_LUMINANCE;
			size_ = width*height*1;
			break;
		default:
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("format"));
		}
	}

	~implementation()
	{
		if(pbo_ != 0)
			glDeleteBuffers(1, &pbo_);
	}	

	void bind_pbo(GLenum mode)
	{
		if(pbo_ == 0)
			GL(glGenBuffers(1, &pbo_));
		GL(glBindBuffer(mode, pbo_));
	}

	void unbind_pbo(GLenum mode)
	{
		GL(glBindBuffer(mode, 0));
	}
	
	void bind_texture()
	{
		if(texture_ == 0)
		{
			GL(glGenTextures(1, &texture_));

			GL(glBindTexture(GL_TEXTURE_2D, texture_));

			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			GL(glTexImage2D(GL_TEXTURE_2D, 0, internal_, width_, height_, 0, format_, 
								GL_UNSIGNED_BYTE, NULL));
		}
		GL(glBindTexture(GL_TEXTURE_2D, texture_));
	}

	void begin_write()
	{
		bind_pbo(GL_PIXEL_UNPACK_BUFFER);
		if(mapped_)
			GL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
		mapped_ = false;
		bind_texture();
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format_, 
							GL_UNSIGNED_BYTE, NULL));
		unbind_pbo(GL_PIXEL_UNPACK_BUFFER);
		writing_ = true;
	}

	void* end_write()
	{
		if(mapped_)
			BOOST_THROW_EXCEPTION(invalid_operation());

		bind_pbo(GL_PIXEL_UNPACK_BUFFER);
		GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
		auto data = static_cast<unsigned char*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
		unbind_pbo(GL_PIXEL_UNPACK_BUFFER);		
		if(!data)
			BOOST_THROW_EXCEPTION(invalid_operation() 
									<< msg_info("glMapBuffer failed"));
		writing_ = false;
		mapped_ = true;
		return data;
	}
	
	void begin_read()
	{	
		bind_pbo(GL_PIXEL_PACK_BUFFER);
		if(mapped_)
			GL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));	
		mapped_ = false;
		GL(glBufferData(GL_PIXEL_PACK_BUFFER, size_, NULL, GL_STREAM_READ));	
		GL(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
		unbind_pbo(GL_PIXEL_PACK_BUFFER);
		reading_ = true;
	}

	void* end_read()
	{
		if(mapped_)
			BOOST_THROW_EXCEPTION(invalid_operation());

		bind_pbo(GL_PIXEL_PACK_BUFFER);
		auto data = static_cast<unsigned char*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));   
		unbind_pbo(GL_PIXEL_PACK_BUFFER);
		if(!data)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
		reading_ = false;
		mapped_ = true;
		return data;
	}

	
	GLuint pbo_;
	GLuint texture_;
	size_t width_;
	size_t height_;
	size_t size_;

	bool mapped_;
	bool writing_;
	bool reading_;

	GLint internal_;
	GLenum format_;
};

pixel_buffer_object::pixel_buffer_object(){}
pixel_buffer_object::pixel_buffer_object(size_t width, size_t height, GLenum format) 
	: impl_(new implementation(width, height, format)){}
void pixel_buffer_object::create(size_t width, size_t height, GLenum format)
{
	impl_.reset(new implementation(width, height, format));
}
void pixel_buffer_object::begin_write() { impl_->begin_write();}
void* pixel_buffer_object::end_write() {return impl_->end_write();} 
void pixel_buffer_object::begin_read() { impl_->begin_read();}
void* pixel_buffer_object::end_read(){return impl_->end_read();}
void pixel_buffer_object::bind_texture() {impl_->bind_texture();}
size_t pixel_buffer_object::width() const {return impl_->width_;}
size_t pixel_buffer_object::height() const {return impl_->height_;}
size_t pixel_buffer_object::size() const {return impl_->size_;}
bool pixel_buffer_object::is_reading() const { return impl_->reading_;}
bool pixel_buffer_object::is_writing() const { return impl_->writing_;}
}}}