#pragma once

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

namespace caspar { namespace protocol { 
	
safe_ptr<core::frame_producer> load_media(const std::vector<std::wstring>& params);

}}
