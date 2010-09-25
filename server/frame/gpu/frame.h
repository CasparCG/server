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

	void lock();
	void unlock();
	void draw();
	
	unsigned char* data();
	size_t size() const;
	size_t width() const;
	size_t height() const;
	void* tag() const;

private:
	void* tag_;
	unsigned char* data_;
	common::gpu::pixel_buffer buffer_;
	common::gpu::texture texture_;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;

}}