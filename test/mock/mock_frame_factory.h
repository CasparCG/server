#pragma once

#include <core/frame/frame_factory.h>
#include <core/frame/gpu_frame.h>

struct mock_frame_factory : public caspar::core::frame_factory
{
	virtual gpu_frame_ptr create_frame(size_t width, size_t height)
	{
		return std::make_shared<mock_frame>();
	}
};