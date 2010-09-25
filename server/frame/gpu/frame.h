#pragma once

#include "../frame.h"

#include "../../../common/gpu/pixel_buffer.h"

#include <memory>

namespace caspar { namespace gpu {
	
class gpu_frame : public frame
{
public:
	gpu_frame(size_t width, size_t height);

	void unmap();
	void map();
	void reset();
	
	void write_to_texture(common::gpu::texture& texture);
	
	unsigned char* data();
	size_t size() const;
	size_t width() const;
	size_t height() const;

private:
	unsigned char* data_;
	common::gpu::pixel_buffer buffer_;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;

}}