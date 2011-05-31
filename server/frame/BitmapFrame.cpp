#include "..\StdAfx.h"
#include "BitmapFrame.h"
#include "BitmapFrameManager.h"

namespace caspar{

BitmapFrame::BitmapFrame(BitmapHolderPtr pBitmap, const utils::ID& factoryID) : pBitmap_(pBitmap), factoryID_(factoryID)
{
}

BitmapFrame::~BitmapFrame()
{
}
unsigned int BitmapFrame::GetDataSize() const {
	return pBitmap_->Size();
}

unsigned char* BitmapFrame::GetDataPtr() const {
	if(pBitmap_ != 0) {
		HasVideo(true);
		return pBitmap_->GetPtr();
	}
	return 0;
}

BitmapHolderPtr BitmapFrame::GetBitmap() const
{
	return pBitmap_;
}

bool BitmapFrame::HasValidDataPtr() const {
	return (pBitmap_ != 0 && pBitmap_->GetPtr() != 0);
}

FrameMetadata BitmapFrame::GetMetadata() const {
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