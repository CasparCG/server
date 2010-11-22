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
 
#include "..\stdafx.h"
#include "BitmapFrameManagerAdapter.h"
#include "BitmapFrameAdapter.h"

#include "..\utils\Lockable.h"
#include "..\utils\BitmapHolder.h"

#include <algorithm>

namespace caspar {

struct BitmapFrameManagerAdapter::Implementation
{
	Implementation(BitmapFrameManagerAdapter* self, FrameManagerPtr pFrameManager, HWND hWnd) : self_(self), pFrameManager_(pFrameManager), hWnd_(hWnd)
	{
	}

	BitmapFramePtr CreateBitmapFrame()
	{
		return BitmapFramePtr(new BitmapFrameAdapter(hWnd_, pFrameManager_->GetFrameFormatDescription().width, pFrameManager_->GetFrameFormatDescription().height, pFrameManager_->CreateFrame(), self_->ID()));
	}

	BitmapFrameManagerAdapter* self_;
	const HWND hWnd_;
	const FrameManagerPtr pFrameManager_;
};

BitmapFrameManagerAdapter::BitmapFrameManagerAdapter(FrameManagerPtr pFrameManager, HWND hWnd) : BitmapFrameManager(pFrameManager->GetFrameFormatDescription(), hWnd), pImpl_(new Implementation(this, pFrameManager, hWnd))
{}

BitmapFrameManagerAdapter::~BitmapFrameManagerAdapter()
{}

BitmapFramePtr BitmapFrameManagerAdapter::CreateBitmapFrame()
{
	return pImpl_->CreateBitmapFrame();
}

}	//namespace caspar