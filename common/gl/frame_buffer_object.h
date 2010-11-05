#pragma once

#include <Glee.h>

#include <memory>

namespace caspar { namespace common { namespace gl {

class frame_buffer_object
{
public:
	frame_buffer_object();
	frame_buffer_object(size_t width, size_t height, GLenum mode = GL_COLOR_ATTACHMENT0_EXT);
	void create(size_t width, size_t height, GLenum mode = GL_COLOR_ATTACHMENT0_EXT);
	void bind_pixel_source();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}