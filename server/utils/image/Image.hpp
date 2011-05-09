/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "../Types.hpp"
#include "../CPUID.hpp"

namespace caspar{

class TransitionInfo;

namespace utils{
namespace image{	
	
/*
   Function: SetVersion

   Sets appropriate function pointers for image library functions depending on SIMD version.

   Modified: 2009/4/14 (R.N)

   Parameters:

      simd	- SIMD version to use

   Author: Robert Nagy, R.N (SVT 2009)
		
*/
void SetVersion(SIMD simd = AUTO);

/*
   Function: Shuffle

   Shuffles 8 byte channels in image

   Modified: 2009/4/15 (R.N)

   Parameters:

      source1	- Image source
	  dest		- Image destination
	  size		- Size of image in bytes
	  mask0		- index of first channel
	  mask1		- index of second channel
	  mask2		- index of third channel
	  mask3		- index of fourth channel

   Author: Robert Nagy, R.N (SVT 2009)
		
   See: Shuffle.hpp	
*/
void Shuffle(void* dest, const void* source, size_t size, const u8 red, const u8 green, const u8 blue, const u8 alpha);

/*
   Function: PreOver

   Blends two images with premultiplied alpha.
   Result is put into dest as an image with premultiplied alpha.

   Modified: 2009/4/12 (R.N)

   Parameters:

      source1	- Image above
      source2	- Image beneath
	  dest		- Image destination
	  size		- Size of image in bytes

   Author: Robert Nagy, R.N (SVT 2009)

   See: Over.hpp
		
*/
void PreOver(void* dest, const void* source1, const void* source2, size_t size);

/*
   Function: Lerp

   Blends two images based on alpha value;
   Result is put into dest as an image with premultiplied alpha.

   Modified: 2009/4/12 (R.N)
			 2009/4/20 (R.N)	

   Parameters:

      source1	- Image above
      source2	- Image beneath
	  dest		- Image destination
	  size		- Size of image in bytes

   Author: Robert Nagy, R.N (SVT 2009)

   See: Lerp.hpp		
*/
void Lerp(void* dest, const void* source1, const void* source2, float alpha, size_t size);

/*
   Function: Premultiply

   Premultiplies color with alpha.

   Modified: 2009/4/20 (R.N)

   Parameters:

      source1	- Image
	  dest		- Image destination
	  size		- Size of image in bytes

   Author: Robert Nagy, R.N (SVT 2009)

   See: Premultiply.hpp		
*/
void Premultiply(void* dest, const void* source, size_t size);

void Copy(void* dest, const void* source, size_t size);

void Clear(void* dest, size_t size);

void CopyField(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);

//void Transition(void* dest, const void* source1, const void* source2, int currentFrame, size_t width, size_t height, const TransitionInfo& transitionInfo);

//void Wipe(void* dest, const void* source1, const void* source2, u32 offset, u32 halfStep, Direction dir, size_t width, size_t height, size_t borderWidth, const void* border = NULL, u32 borderColor = 0);

namespace detail
{
	typedef void(*PreOverFun)		(void*, const void*, const void*, size_t);
	typedef void(*ShuffleFun)		(void*, const void*, size_t, const u8, const u8, const u8, const u8);
	typedef void(*LerpFun)			(void*, const void*, const void*, float, size_t);
	typedef void(*PremultiplyFun)	(void*, const void*, size_t);
	typedef void(*CopyFun)			(void*, const void*, size_t);
	typedef void(*CopyFieldFun)		(unsigned char* pDest, unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
	typedef void(*ClearFun)			(void*, size_t);
	//typedef void(*TransitionFun)(void*, const void*, const void*, int, size_t, size_t, const TransitionInfo&);

	extern ShuffleFun		Shuffle;
	extern PreOverFun		PreOver;
	extern LerpFun			Lerp;
	extern PremultiplyFun	Premultiply;
	extern CopyFun			Copy;
	extern CopyFieldFun		CopyField;
	extern ClearFun			Clear;
	//extern TransitionFun	Transition;

	extern const struct Initializer{Initializer(){SetVersion(AUTO);}} init;
}

} // namespace image
} // namespace utils
} // namespace caspar

#endif