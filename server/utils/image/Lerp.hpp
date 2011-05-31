#ifndef _LERP_H_
#define _LERP_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
		
void Lerp_SSE2(void* dest, const void* source1, const void* source2, float alpha, size_t size);
void Lerp_REF (void* dest, const void* source1, const void* source2, float alpha, size_t size);
void Lerp_OLD (void* dest, const void* source1, const void* source2, float alpha, size_t size);

typedef void(*LerpFun)(void*, const void*, const void*, float, size_t);
LerpFun GetLerpFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


