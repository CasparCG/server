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
#include "BitmapFrameAdapter.h"
#include "BitmapFrameManager.h"

namespace caspar{

struct BitmapHolderAdapter : public caspar::BitmapHolder
{
	BitmapHolderAdapter(HWND hWnd, unsigned int width, unsigned int height, FramePtr pFrame) : BitmapHolder(hWnd, width, height, pFrame->GetDataPtr()), pFrame_(pFrame)
	{
		assert(pFrame->GetDataSize() == width*height*4);
	}
	FramePtr pFrame_;
};

BitmapFrameAdapter::BitmapFrameAdapter(HWND hWnd, unsigned int width, unsigned int height, FramePtr pFrame, const utils::ID& factoryID) : BitmapFrame(BitmapHolderPtr(new BitmapHolderAdapter(hWnd, width, height, pFrame)), factoryID)
{
}

BitmapFrameAdapter::~BitmapFrameAdapter()
{
}

}