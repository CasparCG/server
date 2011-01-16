#pragma once

#include <core/producer/frame_producer.h>
#include <core/consumer/frame_consumer.h>

#include <string>
#include <vector>

namespace caspar { namespace protocol { 
	
safe_ptr<core::frame_producer> create_producer(const std::vector<std::wstring>& params);
safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params);

}}
