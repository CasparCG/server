#pragma once

#include <core/video/frame_factory.h>
#include <core/processor/frame.h>

struct mock_frame_factory : public caspar::core::frame_factory
{
	virtual frame_ptr create_frame(size_t width, size_t height)
	{
		return std::make_shared<mock_frame>();
	}
};