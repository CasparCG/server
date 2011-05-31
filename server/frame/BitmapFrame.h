#ifndef _BITMAP_FRAME_
#define _BITMAP_FRAME_

#include "..\utils\BitmapHolder.h"
#include "Frame.h"

namespace caspar{

class BitmapFrame : public Frame
{
public:
	explicit BitmapFrame(BitmapHolderPtr bitmap, const utils::ID& factoryID);
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

