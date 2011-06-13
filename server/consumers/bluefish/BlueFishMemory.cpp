#include "../../stdafx.h"

#ifndef DISABLE_BLUEFISH

#include "BluefishMemory.h"

namespace caspar { namespace bluefish {
	
std::unordered_map<void*, size_t> page_locked_allocator::size_map_;
size_t page_locked_allocator::free_ = 0;
utils::LockableObject page_locked_allocator::mutex_;

}}

#endif