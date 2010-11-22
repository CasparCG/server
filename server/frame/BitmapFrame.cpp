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
 
#include "..\StdAfx.h"
#include "BitmapFrame.h"
#include "BitmapFrameManager.h"

namespace caspar{

BitmapFrame::BitmapFrame(BitmapHolderPtr pBitmap, const utils::ID& factoryID)
	: pBitmap_(pBitmap), factoryID_(factoryID)
{
}

BitmapFrame::BitmapFrame(const utils::ID& factoryID, HWND hWnd, size_t height, size_t width, void* memory)
	: pBitmap_(new BitmapHolder(hWnd, height, width, memory)), factoryID_(factoryID)
{
}

BitmapFrame::~BitmapFrame()
{
}

unsigned int BitmapFrame::GetDataSize() const 
{
	return pBitmap_->Size();
}

unsigned char* BitmapFrame::GetDataPtr() const 
{
	if(pBitmap_ != 0) 
	{
		HasVideo(true);
		return pBitmap_->GetPtr();
	}
	return 0;
}

BitmapHolderPtr BitmapFrame::GetBitmap() const
{
	return pBitmap_;
}

bool BitmapFrame::HasValidDataPtr() const 
{
	return (pBitmap_ != 0 && pBitmap_->GetPtr() != 0);
}

FrameMetadata BitmapFrame::GetMetadata() const 
{
	return (pBitmap_ != 0) ? reinterpret_cast<FrameMetadata>(pBitmap_->GetDC()) : 0;
}

const utils::ID& BitmapFrame::FactoryID() const
{
	return factoryID_;
}

HDC BitmapFrame::GetDC() const
{
	return pBitmap_->GetDC();
}

}