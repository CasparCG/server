#ifndef OVER_H_
#define OVER_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
		
void PreOver_SSE2(void* dest, const void* source1, const void* source2, size_t size);
void PreOver_FastSSE2(void* dest, const void* source1, const void* source2, size_t size);
void PreOver_REF(void* dest, const void* source1, const void* source2, size_t size);

typedef void(*PreOverFun)(void*, const void*, const void*, size_t);
PreOverFun GetPreOverFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


