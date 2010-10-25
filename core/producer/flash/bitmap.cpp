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
 
#include "../../StdAfx.h"

#include "bitmap.h"

#include <windows.h>

namespace caspar{
	
struct bitmap::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) 
		: size_(width*height*4), width_(width), height_(height), hdc_(CreateCompatibleDC(nullptr)), bitmap_handle_(nullptr)
	{	
		if(hdc_ == nullptr)
			throw std::bad_alloc();
		
		BITMAPINFO bitmapInfo;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biClrImportant = 0;
		bitmapInfo.bmiHeader.biClrUsed = 0;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
	#pragma warning(disable:4146)
		bitmapInfo.bmiHeader.biHeight = -height;
	#pragma warning(default:4146)
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biSizeImage = 0;
		bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
		bitmapInfo.bmiHeader.biYPelsPerMeter = 0;

		bitmap_handle_ = CreateDIBSection(hdc_, &bitmapInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&bitmap_data_), NULL, 0);
		SelectObject(hdc_, bitmap_handle_);	
	}
	
	~implementation()
	{
		if(bitmap_handle_ != nullptr) 
			DeleteObject(bitmap_handle_);
		if(hdc_ != nullptr)
			DeleteDC(hdc_);
	}
	
	const size_t size_;
	const size_t width_;
	const size_t height_;
	unsigned char* bitmap_data_;
	HDC hdc_;
	HBITMAP bitmap_handle_;
};

bitmap::bitmap(size_t width, size_t height) : impl_(new implementation(width, height)){}
size_t bitmap::size() const { return impl_->size_; }
size_t bitmap::width() const { return impl_->width_; }
size_t bitmap::height() const { return impl_->height_; }
unsigned char* bitmap::data() { return impl_->bitmap_data_; }
HDC bitmap::hdc() { return impl_->hdc_; }

}