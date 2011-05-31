#include "..\..\stdafx.h"

#include "Image.hpp"

#include "Over.hpp"
#include "Lerp.hpp"
#include "Shuffle.hpp"
#include "Premultiply.hpp"
#include "Copy.hpp"
#include "Clear.hpp"
//#include "Transition.hpp"

namespace caspar{
namespace utils{
namespace image{

namespace detail
{
	ShuffleFun			Shuffle			= GetShuffleFun(REF);
	PreOverFun			PreOver			= GetPreOverFun(REF);
	LerpFun				Lerp			= GetLerpFun(REF);
	PremultiplyFun		Premultiply		= GetPremultiplyFun(REF);
	CopyFun				Copy			= GetCopyFun(REF);
	CopyFieldFun		CopyField		= GetCopyFieldFun(REF);
	ClearFun			Clear			= GetClearFun(REF);

	const Initializer init;
}

void Shuffle(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha)
{
	(*detail::Shuffle)(dest, source, size, red, green, blue, alpha);
}

void PreOver(void* dest, const void* source1, const void* source2, size_t size)
{
	(*detail::PreOver)(dest, source1, source2, size);
}

void Lerp(void* dest, const void* source1, const void* source2, float alpha, size_t size)
{
	(*detail::Lerp)(dest, source1, source2, alpha, size);
}

void Premultiply(void* dest, const void* source, size_t size)
{
	(*detail::Premultiply)(dest, source, size);
}

void Copy(void* dest, const void* source, size_t size)
{
	(*detail::Copy)(dest, source, size);
}

void CopyField(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height)
{
	(*detail::CopyField)(pDest, pSrc, fieldIndex, width, height);
}

void Clear(void* dest, size_t size)
{
	(*detail::Clear)(dest, size);
}
/*
void Transition(void* dest, const void* source1, const void* source2, int currentFrame, size_t width, size_t height, const TransitionInfo& transitionInfo)
{
	(*detail::Transition)(dest, source1, source2, currentFrame, width, height, transitionInfo);
}*/

void SetVersion(SIMD simd)
{	
	if(simd == AUTO)
		simd = CPUID().SIMD;
	
	detail::Shuffle			= GetShuffleFun(simd);
	detail::PreOver			= GetPreOverFun(simd);
	detail::Lerp			= GetLerpFun(simd);
	detail::Premultiply		= GetPremultiplyFun(simd);
	detail::Copy			= GetCopyFun(simd);
	detail::CopyField		= GetCopyFieldFun(simd);
	detail::Clear			= GetClearFun(simd);
	//detail::Transition		= GetTransitionFun(simd);
}

} // namespace image
} // namespace utils
} // namespace caspar