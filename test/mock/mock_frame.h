#pragma once

#include <core/frame/gpu_frame.h>

struct mock_frame : public caspar::core::gpu_frame
{
	mock_frame(void* tag, short volume = 100) : caspar::core::gpu_frame(0,0), tag(tag)
	{
		audio_data().resize(100, volume);
	}
	void* tag;
};