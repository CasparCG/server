#pragma once

#include "../producer/frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { 
	
safe_ptr<frame_producer> load_media(const std::vector<std::wstring>& params);

}}
