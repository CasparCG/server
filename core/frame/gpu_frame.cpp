#include "../StdAfx.h"

#include "gpu_frame.h"
#include "../../common/image/copy.h"
#include "../../common/gl/utility.h"

namespace caspar {
	
struct gpu_frame::implementation
{
	implementation(size_t width, size_t height) 
		: pbo_(0), data_(nullptr), width_(width), height_(height), size_(width*height*4), reading_(false), texture_(0), alpha_(1.0f)
	{	
	}

	~implementation()
	{
		if(pbo_ != 0)
			glDeleteBuffers(1, &pbo_);
		if(texture_ != 0)
			glDeleteTextures(1, &texture_);
	}

	GLuint pbo()
	{		
		if(pbo_ == 0)
			CASPAR_GL_CHECK(glGenBuffers(1, &pbo_));
		return pbo_;
	}

	void write_lock()
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

		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo()));
		if(data_ != nullptr)
		{
			CASPAR_GL_CHECK(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
			data_ = nullptr;
		}
		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
		CASPAR_GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	}

	bool write_unlock()
	{
		if(data_ != nullptr)
			return false;
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo()));
		CASPAR_GL_CHECK(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
		void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
		data_ = reinterpret_cast<unsigned char*>(ptr);
		if(!data_)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
		return true;
	}
	
	void read_lock(GLenum mode)
	{	
		CASPAR_GL_CHECK(glReadBuffer(mode));
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo()));
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

	bool read_unlock()
	{
		if(data_ != nullptr || !reading_)
			return false;
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo()));
		void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);   
		CASPAR_GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
		data_ = reinterpret_cast<unsigned char*>(ptr);
		if(!data_)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
		reading_ = false;
		return true;
	}

	void draw()
	{
		glPushMatrix();
		glTranslatef(x_*2.0f, y_*2.0f, 0.0f);
		glColor4f(1.0f, 1.0f, 1.0f, alpha_);
		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
		glEnd();
		glPopMatrix();
	}
		
	unsigned char* data()
	{
		if(data_ == nullptr)
			BOOST_THROW_EXCEPTION(invalid_operation());
		return data_;
	}

	void reset()
	{
		audio_data_.clear();
		alpha_ = 1.0f;
		x_ = 0.0f;
		y_ = 0.0f;
	}

	gpu_frame* self_;
	GLuint pbo_;
	GLuint texture_;
	unsigned char* data_;
	size_t width_;
	size_t height_;
	size_t size_;
	bool reading_;
	std::vector<short> audio_data_;

	float alpha_;
	float x_;
	float y_;
};

gpu_frame::gpu_frame(size_t width, size_t height) : impl_(new implementation(width, height)){}
void gpu_frame::write_lock(){impl_->write_lock();}
bool gpu_frame::write_unlock(){return impl_->write_unlock();}	
void gpu_frame::read_lock(GLenum mode){impl_->read_lock(mode);}
bool gpu_frame::read_unlock(){return impl_->read_unlock();}
void gpu_frame::draw(){impl_->draw();}
unsigned char* gpu_frame::data(){return impl_->data();}
size_t gpu_frame::size() const { return impl_->size_; }
size_t gpu_frame::width() const { return impl_->width_;}
size_t gpu_frame::height() const { return impl_->height_;}
const std::vector<short>& gpu_frame::audio_data() const { return impl_->audio_data_; }	
std::vector<short>& gpu_frame::audio_data() { return impl_->audio_data_; }
void gpu_frame::reset(){impl_->reset();}
float gpu_frame::alpha() const{ return impl_->alpha_;}
void gpu_frame::alpha(float value){ impl_->alpha_ = value;}
float gpu_frame::x() const { return impl_->x_;}
float gpu_frame::y() const { return impl_->y_;}
void gpu_frame::translate(float x, float y) { impl_->x_ += x; impl_->y_ += y; }
}