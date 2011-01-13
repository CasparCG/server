#pragma once

#include "../frame_producer.h"

#include <common/memory/safe_ptr.h>

#include <string>
#include <vector>

namespace caspar { namespace core { namespace ffmpeg {
	
safe_ptr<frame_producer> create_ffmpeg_producer(const std::vector<std::wstring>& params);

}}}