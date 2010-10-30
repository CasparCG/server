#pragma once

#include "../frame/frame_fwd.h"
#include "../producer/frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { 
	
frame_producer_ptr load_media(const std::vector<std::wstring>& params, const frame_format_desc& format_desc);

}}
