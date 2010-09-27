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
 
#include "../StdAfx.h"

#include "bitmap_frame.h"

#include <windows.h>

namespace caspar{
	
struct bitmap_frame::implementation : boost::noncopyable
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
	
	implementation(const std::shared_ptr<frame>& frame)
		: size_(frame->size()), width_(frame->width()), height_(frame->height()), hdc_(CreateCompatibleDC(nullptr)), bitmap_handle_(nullptr), frame_(frame)
	{
		if(hdc_ == nullptr)
			throw std::bad_alloc();

		//bitmap_data_ = frame_->data();
		//BITMAP bmp;
		//bmp.bmBits = bitmap_data_;
		//bmp.bmHeight = frame->height();
		//bmp.bmWidth = frame->width();
		//bmp.bmWidthBytes = frame->width()*4;
		//bmp.bmBitsPixel = 32;
		//bmp.bmPlanes = 1;
		//bmp.bmType = 0;
		//bitmap_handle_ = CreateBitmapIndirect(&bmp);
		//bitmap_data_ = frame->data();
		//SelectObject(hdc_, bitmap_handle_);	

		
		BITMAPINFO bitmapInfo;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biClrImportant = 0;
		bitmapInfo.bmiHeader.biClrUsed = 0;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
	#pragma warning(disable:4146)
		bitmapInfo.bmiHeader.biHeight = -height_;
	#pragma warning(default:4146)
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
		bitmapInfo.bmiHeader.biWidth = width_;
		bitmapInfo.bmiHeader.biSizeImage = 0;
		bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
		bitmapInfo.bmiHeader.biYPelsPerMeter = 0;

		bitmap_handle_ = CreateCompatibleBitmap(hdc_, width_, height_);
		SelectObject(hdc_, bitmap_handle_);	
		SetDIBits(hdc_, bitmap_handle_, 0, height_, frame_->data(), &bitmapInfo, 0);
		bitmap_data_ = frame_->data();
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

	std::shared_ptr<frame> frame_;
};

bitmap_frame::bitmap_frame(size_t width, size_t height) : impl_(new implementation(width, height)){}
bitmap_frame::bitmap_frame(const std::shared_ptr<frame>& frame) : impl_(new implementation(frame)){}
size_t bitmap_frame::size() const { return impl_->size_; }
size_t bitmap_frame::width() const { return impl_->width_; }
size_t bitmap_frame::height() const { return impl_->height_; }
unsigned char* bitmap_frame::data() { return impl_->bitmap_data_; }
HDC bitmap_frame::hdc() { return impl_->hdc_; }

}