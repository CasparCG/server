#include "../StdAfx.h"

#include "gpu_frame.h"
#include "../../common/utility/memory.h"
#include "../../common/gl/gl_check.h"

namespace caspar { namespace core {
	
GLubyte progressive_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xFF, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
GLubyte upper_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00};
		
GLubyte lower_pattern[] = {
							0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff};
																																						
struct gpu_frame::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) 
		: pbo_(0), data_(nullptr), width_(width), height_(height), 
			size_(width*height*4), reading_(false), texture_(0), alpha_(1.0f), 
			x_(0.0f), y_(0.0f), mode_(video_mode::progressive), 
			texcoords_(0.0, 1.0, 1.0, 0.0)
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
			GL(glGenBuffers(1, &pbo_));
		return pbo_;
	}

	void write_lock()
	{
		if(texture_ == 0)
		{
			GL(glGenTextures(1, &texture_));

			GL(glBindTexture(GL_TEXTURE_2D, texture_));

			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, 
								GL_UNSIGNED_BYTE, NULL));
		}

		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo()));
		if(data_ != nullptr)
		{
			GL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
			data_ = nullptr;
		}
		GL(glBindTexture(GL_TEXTURE_2D, texture_));
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_BGRA, 
							GL_UNSIGNED_BYTE, NULL));
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	}

	bool write_unlock()
	{
		if(data_ != nullptr)
			return false;
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo()));
		GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW));
		void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
		data_ = reinterpret_cast<unsigned char*>(ptr);
		if(!data_)
			BOOST_THROW_EXCEPTION(invalid_operation() 
									<< msg_info("glMapBuffer failed"));
		return true;
	}
	
	void read_lock(GLenum mode)
	{	
		GL(glReadBuffer(mode));
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo()));
		if(data_ != nullptr)	
		{	
			GL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));	
			data_ = nullptr;
		}
		GL(glBufferData(GL_PIXEL_PACK_BUFFER, size_, NULL, GL_STREAM_READ));	
		GL(glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
		reading_ = true;
	}

	bool read_unlock()
	{
		if(data_ != nullptr || !reading_)
			return false;
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo()));
		void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);   
		GL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
		data_ = reinterpret_cast<unsigned char*>(ptr);
		if(!data_)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
		reading_ = false;
		return true;
	}

	void draw()
	{
		glPushMatrix();
		glTranslated(x_*2.0, y_*2.0, 0.0);
		glColor4d(1.0, 1.0, 1.0, alpha_);

		if(mode_ == video_mode::progressive)
			glPolygonStipple(progressive_pattern);
		else if(mode_ == video_mode::upper)
			glPolygonStipple(upper_pattern);
		else if(mode_ == video_mode::lower)
			glPolygonStipple(lower_pattern);

		GL(glBindTexture(GL_TEXTURE_2D, texture_));
		glBegin(GL_QUADS);
			glTexCoord2d(texcoords_.left,	texcoords_.bottom); glVertex2d(-1.0, -1.0);
			glTexCoord2d(texcoords_.right,	texcoords_.bottom); glVertex2d( 1.0, -1.0);
			glTexCoord2d(texcoords_.right,	texcoords_.top);	glVertex2d( 1.0,  1.0);
			glTexCoord2d(texcoords_.left,	texcoords_.top);	glVertex2d(-1.0,  1.0);
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
		alpha_     = 1.0f;
		x_         = 0.0f;
		y_         = 0.0f;
		texcoords_ = rectangle(0.0, 1.0, 1.0, 0.0);
		mode_      = video_mode::progressive;
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

	double alpha_;
	double x_;
	double y_;
	video_mode mode_;
	rectangle texcoords_;
};

gpu_frame::gpu_frame(size_t width, size_t height) 
	: impl_(new implementation(width, height)){}
void gpu_frame::write_lock(){impl_->write_lock();}
bool gpu_frame::write_unlock(){return impl_->write_unlock();}	
void gpu_frame::read_lock(GLenum mode){impl_->read_lock(mode);}
bool gpu_frame::read_unlock(){return impl_->read_unlock();}
void gpu_frame::draw(){impl_->draw();}
unsigned char* gpu_frame::data(){return impl_->data();}
size_t gpu_frame::size() const { return impl_->size_; }
size_t gpu_frame::width() const { return impl_->width_;}
size_t gpu_frame::height() const { return impl_->height_;}
const std::vector<short>& gpu_frame::audio_data() const{return impl_->audio_data_;}	
std::vector<short>& gpu_frame::audio_data() { return impl_->audio_data_; }
void gpu_frame::reset(){impl_->reset();}
double gpu_frame::alpha() const{ return impl_->alpha_;}
void gpu_frame::alpha(double value){ impl_->alpha_ = value;}
double gpu_frame::x() const { return impl_->x_;}
double gpu_frame::y() const { return impl_->y_;}
void gpu_frame::translate(double x, double y) { impl_->x_ += x; impl_->y_ += y; }
void gpu_frame::texcoords(const rectangle& texcoords){impl_->texcoords_ = texcoords;}
void gpu_frame::mode(video_mode mode){ impl_->mode_ = mode;}
video_mode gpu_frame::mode() const{ return impl_->mode_;}
}}