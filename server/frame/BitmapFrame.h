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
 
#ifndef _BITMAP_FRAME_
#define _BITMAP_FRAME_

#include "..\utils\BitmapHolder.h"
#include "Frame.h"

namespace caspar{

class BitmapFrame : public Frame
{
public:
	explicit BitmapFrame(BitmapHolderPtr bitmap, const utils::ID& factoryID);
	BitmapFrame(const utils::ID& factoryID, HWND hWnd, size_t height, size_t width, void* memory = NULL);
	virtual ~BitmapFrame();

	virtual unsigned char* GetDataPtr() const;
	virtual bool HasValidDataPtr() const;
	virtual unsigned int GetDataSize() const;
	virtual FrameMetadata GetMetadata() const;

	HDC GetDC() const;
	BitmapHolderPtr GetBitmap() const;

	const utils::ID& FactoryID() const;	

private:
	size_t size_;
	BitmapHolderPtr pBitmap_;
	utils::ID factoryID_;
};
typedef std::tr1::shared_ptr<BitmapFrame> BitmapFramePtr;

}

#endif

