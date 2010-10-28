#pragma once

namespace caspar { namespace common {
		
void* copy(void* dest, const void* source, size_t size);
void* clear(void* dest, size_t size);

}}