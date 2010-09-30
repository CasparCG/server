#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace ffmpeg {
	
frame_producer_ptr create_ffmpeg_producer(const  std::vector<std::wstring>& params, const frame_format_desc& format_desc);

}}