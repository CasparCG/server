#pragma once

#include "../frame.h"

#include "../../../common/gpu/pixel_buffer.h"
#include "../../../common/gpu/texture.h"

#include <memory>

namespace caspar { namespace gpu {
	
class gpu_frame : public frame
{
public:
	gpu_frame(size_t width, size_t height, void* tag);
	~gpu_frame();

	void write_lock();
	bool write_unlock();
	void read_lock(GLenum mode);
	bool read_unlock();
	void draw();
		
	unsigned char* data();
	size_t size() const;
	size_t width() const;
	size_t height() const;
	void* tag() const;

	GLuint pbo() { return pbo_; }

private:
	GLuint pbo_;
	GLuint texture_;
	void* tag_;
	unsigned char* data_;
	size_t width_;
	size_t height_;
	size_t size_;
	bool reading_;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;

}}