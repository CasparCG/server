#ifndef _BITMAP_HOLDER_
#define _BITMAP_HOLDER_

#include <memory>
#include "..\frame\Frame.h"
#include "Noncopyable.hpp"


namespace caspar{

class BitmapHolder;
typedef std::tr1::shared_ptr<BitmapHolder> BitmapHolderPtr;

class BitmapHolder : private utils::Noncopyable
{
public:
	BitmapHolder(HWND hWnd, size_t height, size_t width);
	~BitmapHolder();

	HDC GetDC() const {
		return hDC_;
	}
	unsigned char* GetPtr() const {
		return pBitmapData_;
	}

	size_t Width() const
	{
		return width_;
	}
	size_t Height() const
	{
		return height_;
	}

	size_t Size() const
	{
		return width_*height_*4;
	}

private:

	static bool CreateBitmap(HDC dc, size_t width, size_t height, HBITMAP& hBitmap, void** pBitmapData);

	size_t width_;
	size_t height_;
	HDC hDC_;
	HBITMAP hBitmap_;
	unsigned char* pBitmapData_;
};

}

#endif

