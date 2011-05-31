#ifndef _SHUFFLE_
#define _SHUFFLE_

#include "../CPUID.hpp"
#include "../Types.hpp"

namespace caspar{
namespace utils{
namespace image{

void Shuffle_SSSE3(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void Shuffle_SSE2 (void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);
void Shuffle_REF  (void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);

typedef void(*ShuffleFun)(void*, const void*, size_t, const u8, const u8, const u8, const u8);
ShuffleFun GetShuffleFun(SIMD simd = REF);

} // namespace image
} // namespace utils
} // namespace caspar

#endif