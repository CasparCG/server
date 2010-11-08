#pragma once

#include "../producer/frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { 
	
frame_producer_ptr load_media(const std::vector<std::wstring>& params);

}}
