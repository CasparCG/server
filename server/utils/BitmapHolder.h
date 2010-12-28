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
 
#ifndef _BITMAP_HOLDER_
#define _BITMAP_HOLDER_

#include <memory>
#include "..\frame\Frame.h"
#include "Noncopyable.hpp"

namespace caspar{

/*
   Class: BitmapHolder   

   Changes: 
   2010/4/2 (R.N), Refactored
   2010/4/3 (R.N), Implemented functionality to map bitmap to allocated memory

   Author: Niklas P. Andersson, N.A		
*/

class BitmapHolder : private utils::Noncopyable
{
public:
	BitmapHolder(HWND hWnd, size_t height, size_t width);

	HDC GetDC() const;
	unsigned char* GetPtr() const;
	size_t Width() const;
	size_t Height() const;
	size_t Size() const;

private:

	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<BitmapHolder> BitmapHolderPtr;

}

#endif

