#pragma once

#include <memory>

#include <Glee.h>

#include "texture.h"
#include "gl_check.h"

namespace caspar { namespace common { namespace gpu {
	
class fullscreen_quad
{
public:

	fullscreen_quad()
	{
		dlist_ = glGenLists(1);

		glNewList(dlist_, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f,	 1.0f); glVertex2f(-1.0f, -1.0f);
				glTexCoord2f(1.0f,	 1.0f);	glVertex2f( 1.0f, -1.0f);
				glTexCoord2f(1.0f,	 0.0f);	glVertex2f( 1.0f,  1.0f);
				glTexCoord2f(0.0f,	 0.0f);	glVertex2f(-1.0f,  1.0f);
			glEnd();	
		glEndList();
	}
	
	void draw() const
	{
		glCallList(dlist_);	
	}

private:
	GLuint dlist_;
};

class write_pixel_buffer
{
public:

	write_pixel_buffer(size_t width, size_t height);
	~write_pixel_buffer();

	void begin_write(void* src);
	void end_write();
	void draw();

	size_t size() const;
	size_t width() const;
	size_t height() const;
	GLuint pbo_handle();
private:
	texture texture_;
	GLuint pbo_;
	size_t size_;
	fullscreen_quad quad_;
	bool written_;
};
typedef std::shared_ptr<write_pixel_buffer> write_pixel_buffer_ptr;

class read_pixel_buffer
{
public:
	read_pixel_buffer(size_t width, size_t height);
	~read_pixel_buffer();
	
	void begin_read();
	void end_read(void* dest);	

	size_t size() const;
	size_t width() const;
	size_t height() const;
	GLuint pbo_handle();
private:
	bool reading_;
	GLuint pbo_;
	size_t size_;
	size_t width_;
	size_t height_;
};
typedef std::shared_ptr<read_pixel_buffer> read_pixel_buffer_ptr;

}}}