#ifndef COPY_H_
#define COPY_H_

#include "../CPUID.hpp"

namespace caspar{
namespace utils{
namespace image{
	
void Copy_SSE2	 (void* dest, const void* source, size_t size);
void Copy_REF	 (void* dest, const void* source, size_t size);

typedef void(*CopyFun)(void*, const void*, size_t);
CopyFun GetCopyFun(SIMD simd = REF);

void CopyField_SSE2	 (unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
void CopyField_REF	 (unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);

typedef void(*CopyFieldFun)(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
CopyFieldFun GetCopyFieldFun(SIMD simd = REF);


//void StraightTransform_SSE2(const void* source, void* dest, size_t size);
//void StraightTransform_REF(const void* source, void* dest, size_t size);
//
//typedef void(*StraightTransformFun)(const void*, void*, size_t);
//StraightTransformFun GetStraightTransformFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif


