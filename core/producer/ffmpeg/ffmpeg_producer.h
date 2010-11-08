#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { namespace ffmpeg {
	
frame_producer_ptr create_ffmpeg_producer(const  std::vector<std::wstring>& params);

}}}