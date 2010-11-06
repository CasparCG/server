#pragma once

#include "gpu_frame.h"
#include "frame_format.h"

#include <memory>
#include <array>

namespace caspar { namespace core { 
	
struct frame_factory
{
	virtual ~frame_factory(){}
	virtual void release_frames(void* tag) = 0;
	virtual gpu_frame_ptr create_frame(size_t width, size_t height, void* tag) = 0;
	virtual gpu_frame_ptr create_frame(const planar_frame_dimension& data_size, void* tag) = 0;
	gpu_frame_ptr create_frame(const frame_format_desc format_desc, void* tag)
	{
		return create_frame(format_desc.width, format_desc.height, tag);
	}
};

typedef std::shared_ptr<frame_factory> frame_factory_ptr;

}}