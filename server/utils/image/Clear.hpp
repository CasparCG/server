#ifndef CLEAR_H_
#define CLEAR_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
	
void Clear_SSE2	 (void* dest, size_t size);
void Clear_REF	 (void* dest, size_t size);

typedef void(*ClearFun)(void*, size_t);
ClearFun GetClearFun(SIMD simd = REF);


//void StraightTransform_SSE2(const void* source, void* dest, size_t size);
//void StraightTransform_REF(const void* source, void* dest, size_t size);
//
//typedef void(*StraightTransformFun)(const void*, void*, size_t);
//StraightTransformFun GetStraightTransformFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


