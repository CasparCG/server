#pragma once

#include <core/processor/frame.h>

struct mock_frame : public caspar::core::frame
{
	mock_frame(void* tag, short volume = 100) : caspar::core::frame(0,0), tag(tag)
	{
		audio_data().resize(100, volume);
	}
	void* tag;
};