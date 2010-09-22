#pragma once

#include <memory>

#include <Glee.h>

#include "texture.h"
#include "gl_check.h"

namespace caspar { namespace common { namespace gpu {
	

class pixel_buffer
{
public:

	pixel_buffer(size_t width, size_t height);
	~pixel_buffer();
	
	void read_to_pbo();
	void read_to_memory(void* dest);	
	
	void write_to_pbo(void* src);
	void write_to_texture(texture& texture);

	size_t size() const;
	size_t width() const;
	size_t height() const;
	GLuint pbo_handle();
private:
	GLuint pbo_;
	size_t size_;
	bool written_;
	bool reading_;
	size_t width_;
	size_t height_;
};
typedef std::shared_ptr<pixel_buffer> pixel_buffer_ptr;


}}}