#include "..\StdAfx.h"

#include "BitmapHolder.h"
#include "DCWrapper.h"

namespace caspar{

BitmapHolder::BitmapHolder(HWND hWnd, size_t width, size_t height): hDC_(0), hBitmap_(0), pBitmapData_(0), width_(width), height_(height)
{
	DCWrapper hDC(hWnd);

	hDC_ = CreateCompatibleDC(hDC);
	if(CreateBitmap(hDC_, width_, height_, hBitmap_, reinterpret_cast<void**>(&pBitmapData_)))
		SelectObject(hDC_, hBitmap_);	
	else
	{
		hDC_ = 0;
		hBitmap_ = 0;
		pBitmapData_ = 0;
		throw std::exception("Failed to create bitmap");
	}
}

BitmapHolder::~BitmapHolder()
{
	if(hBitmap_ != 0) {
		DeleteObject(hBitmap_);
		hBitmap_ = 0;
	}

	if(hDC_ != 0) {
		DeleteDC(hDC_);
		hDC_ = 0;
	}
}

bool BitmapHolder::CreateBitmap(HDC dc, size_t width, size_t height, HBITMAP& hBitmap, void** pBitmapData)
{
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;

	hBitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, pBitmapData, NULL, 0);
	return (hBitmap != 0);
}

}