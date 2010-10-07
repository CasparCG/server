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
#include "BitmapFrameManager.h"
#include "BitmapFrame.h"

#include "..\utils\Lockable.h"
#include "..\utils\BitmapHolder.h"

#include <algorithm>

namespace caspar {

struct BitmapFrameManager::Implementation
{

	Implementation(BitmapFrameManager* self, const FrameFormatDescription& fmtDesc, HWND hWnd) : self_(self), fmtDesc_(fmtDesc), hWnd_(hWnd), pBitmaps_(new LockableBitmapVector())
	{
		features_.push_back("BITMAP_FRAME");
	}

	FramePtr CreateFrame()
	{	
		BitmapFramePtr pBitmapFrame;
		{
			LockableObject::Lock lock(*pBitmaps_);
			if(!pBitmaps_->empty())
			{
				pBitmapFrame = pBitmaps_->back();
				pBitmaps_->pop_back();
			}
		}
		
		if(!pBitmapFrame)		
			pBitmapFrame = self_->CreateBitmapFrame();
		
		class FrameDeallocator
		{
		public:
			FrameDeallocator(BitmapFramePtr pBitmapFrame, const LockableBitmapVectorPtr pBitmaps) : pBitmapFrame_(pBitmapFrame), pBitmaps_(pBitmaps){}
			void operator()(BitmapFrame*)
			{
				LockableObject::Lock lock(*pBitmaps_);
				pBitmaps_->push_back(BitmapFramePtr(new BitmapFrame(pBitmapFrame_->GetBitmap(), pBitmapFrame_->FactoryID())));
			}
		private:
			LockableBitmapVectorPtr pBitmaps_;
			BitmapFramePtr pBitmapFrame_;
		};
		
		return BitmapFramePtr(pBitmapFrame.get(), FrameDeallocator(pBitmapFrame, pBitmaps_));
	}

	const FrameFormatDescription& GetFrameFormatDescription() const 
	{
		return fmtDesc_;
	}

	bool HasFeature(const std::string& feature) const
	{
		return std::find(features_.begin(), features_.end(), feature) != features_.end();
	}
	
	BitmapFramePtr CreateBitmapFrame()
	{
		return BitmapFramePtr(new BitmapFrame(self_->ID(), hWnd_, fmtDesc_.width, fmtDesc_.height));
	}

	// TODO: need proper threading tools (R.N)
	struct LockableBitmapVector : public std::vector<BitmapFramePtr>, public LockableObject{};
	typedef std::tr1::shared_ptr<LockableBitmapVector> LockableBitmapVectorPtr;

	LockableBitmapVectorPtr pBitmaps_;

	std::vector<const std::string> features_;

	const FrameFormatDescription fmtDesc_;	
	const HWND hWnd_;

	BitmapFrameManager* self_;
};

BitmapFrameManager::BitmapFrameManager(const FrameFormatDescription& fmtDesc, HWND hWnd) : pImpl_(new Implementation(this, fmtDesc, hWnd))
{}

BitmapFrameManager::~BitmapFrameManager()
{}

FramePtr BitmapFrameManager::CreateFrame()
{	
	return pImpl_->CreateFrame();
}

const FrameFormatDescription& BitmapFrameManager::GetFrameFormatDescription() const 
{
	return pImpl_->GetFrameFormatDescription();
}

bool BitmapFrameManager::HasFeature(const std::string& feature) const
{
	return pImpl_->HasFeature(feature);
}

BitmapFramePtr BitmapFrameManager::CreateBitmapFrame()
{
	return pImpl_->CreateBitmapFrame();
}

}	//namespace caspar