#pragma once

#include <common/spl/memory.h>

namespace caspar { namespace core {
	
spl::shared_ptr<struct frame_consumer> create_consumer_cadence_guard(const spl::shared_ptr<struct frame_consumer>& consumer);

}}