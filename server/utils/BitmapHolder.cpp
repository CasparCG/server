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

#include "BitmapHolder.h"
#include "DCWrapper.h"

namespace caspar{

bool CreateBitmap(HDC dc, size_t width, size_t height, HBITMAP& hBitmap, void** pBitmapData, void* memory)
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

	if(memory != NULL)
	{		
		HDC memoryDC = CreateCompatibleDC(dc);
		if(memoryDC == NULL)
			return false;

		HANDLE hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, width*height*4, NULL);
		MapViewOfFileEx(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0, memory);

		hBitmap = CreateDIBSection(memoryDC, &bitmapInfo, DIB_RGB_COLORS, pBitmapData, hMapping, 0);
	}
	else
		hBitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, pBitmapData, NULL, 0);

	return (hBitmap != 0);
}

struct BitmapHolder::Implementation
{	

	Implementation(HWND hWnd, size_t width, size_t height, void* memory)
		: hDC_(0), hBitmap_(0), pBitmapData_(0), width_(width), height_(height)
	{
		DCWrapper hDC(hWnd);

		hDC_ = CreateCompatibleDC(hDC);
		if(CreateBitmap(hDC_, width_, height_, hBitmap_, reinterpret_cast<void**>(&pBitmapData_), memory))
			SelectObject(hDC_, hBitmap_);	
		else
		{
			hDC_ = 0;
			hBitmap_ = 0;
			pBitmapData_ = 0;
			throw std::exception("Failed to create bitmap");
		}
	}

	~Implementation()
	{	
		if(hBitmap_ != 0) 
		{
			DeleteObject(hBitmap_);
			hBitmap_ = 0;
		}

		if(hDC_ != 0) 
		{
			DeleteDC(hDC_);
			hDC_ = 0;
		}
	}

	size_t width_;
	size_t height_;
	HDC hDC_;
	HBITMAP hBitmap_;
	unsigned char* pBitmapData_;
};

BitmapHolder::BitmapHolder(HWND hWnd, size_t width, size_t height, void* memory): pImpl_(new Implementation(hWnd, width, height, memory))
{
}

HDC BitmapHolder::GetDC() const {
	return pImpl_->hDC_;
}

unsigned char* BitmapHolder::GetPtr() const {
	return pImpl_->pBitmapData_;
}

size_t BitmapHolder::Width() const
{
	return pImpl_->width_;
}

size_t BitmapHolder::Height() const
{
	return pImpl_->height_;
}

size_t BitmapHolder::Size() const
{
	return pImpl_->width_*pImpl_->height_*4;
}

}