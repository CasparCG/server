#pragma once

namespace caspar { namespace common {
		
void* aligned_memcpy(void* dest, const void* source, size_t size);
void* clear(void* dest, size_t size);

}}