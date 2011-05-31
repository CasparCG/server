#ifndef PREMULTIPLY_H_
#define PREMULTIPLY_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
	
void Premultiply_SSE2	 (void* dest, const void* source, size_t size);
void Premultiply_FastSSE2(void* dest, const void* source, size_t size);
void Premultiply_REF	 (void* dest, const void* source, size_t size);

typedef void(*PremultiplyFun)(void*, const void*, size_t);
PremultiplyFun GetPremultiplyFun(SIMD simd = REF);


//void StraightTransform_SSE2(const void* source, void* dest, size_t size);
//void StraightTransform_REF(const void* source, void* dest, size_t size);
//
//typedef void(*StraightTransformFun)(const void*, void*, size_t);
//StraightTransformFun GetStraightTransformFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


