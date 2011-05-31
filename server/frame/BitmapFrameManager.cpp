#include "..\stdafx.h"
#include "BitmapFrameManager.h"
#include "BitmapFrame.h"

#include <algorithm>

namespace caspar {

class BitmapFrameManager::FrameDeallocator
{
public:
	FrameDeallocator(const LockableBitmapCollectionPtr pBitmaps) : pBitmaps_(pBitmaps){}
	void operator()(Frame* frame)
	{
		LockableObject::Lock lock(*pBitmaps_);
		pBitmaps_->push_back(static_cast<BitmapFrame*>(frame)->GetBitmap());
		delete frame;
	}
private:
	LockableBitmapCollectionPtr pBitmaps_;
};


BitmapFrameManager::BitmapFrameManager(const FrameFormatDescription& fmtDesc, HWND hWnd) : fmtDesc_(fmtDesc), hWnd_(hWnd), pBitmaps_(new LockableBitmapCollection())
{
	features_.push_back("BITMAP_FRAME");
}

BitmapFrameManager::~BitmapFrameManager()
{}

FramePtr BitmapFrameManager::CreateFrame()
{	
	BitmapHolderPtr pBitmap;
	{
		LockableObject::Lock lock(*pBitmaps_);
		if(!pBitmaps_->empty())
		{
			pBitmap = pBitmaps_->back();
			pBitmaps_->pop_back();
		}
	}
	if(!pBitmap)	
		pBitmap = BitmapHolderPtr(new BitmapHolder(hWnd_, fmtDesc_.width, fmtDesc_.height));	
	
	return FramePtr(new BitmapFrame(pBitmap, this->ID()), FrameDeallocator(pBitmaps_));
}

const FrameFormatDescription& BitmapFrameManager::GetFrameFormatDescription() const 
{
	return fmtDesc_;
}

bool BitmapFrameManager::HasFeature(const std::string& feature) const
{
	return std::find(features_.begin(), features_.end(), feature) != features_.end();
}

}	//namespace caspar