#pragma once

#include <core/producer/frame_producer.h>

#include <common/memory/safe_ptr.h>

#include <string>
#include <vector>

namespace caspar {
	
safe_ptr<core::frame_producer> create_ffmpeg_producer(const std::vector<std::wstring>& params);

}