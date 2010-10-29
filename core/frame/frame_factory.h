#pragma once

#include "gpu_frame.h"
#include "frame_format.h"

#include <memory>

namespace caspar { 

struct frame_factory
{
	virtual ~frame_factory(){}
	virtual gpu_frame_ptr create_frame(size_t width, size_t height) = 0;
	gpu_frame_ptr create_frame(const frame_format_desc format_desc)
	{
		return create_frame(format_desc.width, format_desc.height);
	}
};

typedef std::shared_ptr<frame_factory> frame_factory_ptr;

}