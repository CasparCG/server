#pragma once

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
safe_ptr<struct frame_consumer> create_consumer_cadence_guard(const safe_ptr<struct frame_consumer>& consumer);

}}