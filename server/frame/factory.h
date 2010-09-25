#pragma once

#include "frame_fwd.h"

#include <memory>

namespace caspar {

struct frame_factory
{
	virtual frame_ptr create_frame(size_t width, size_t height) = 0;
};

typedef std::shared_ptr<frame_factory> frame_factory_ptr;

}