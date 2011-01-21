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
 
#include "../../stdafx.h"

#include "screen_producer.h"

#include <common/concurrency/executor.h>

#include <mixer/frame/draw_frame.h>

#include <boost/algorithm/string.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace core {
	
class screen_producer : public frame_producer
{
	std::shared_ptr<frame_factory> frame_factory_;
	
	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;
	safe_ptr<draw_frame> head_;
	executor executor_;

	HDC screen_;
	HDC mem_;
	HBITMAP bmp_;
	unsigned char* bmp_data_;
	int x_;
	int y_;
	size_t width_;
	size_t height_;

public:

	explicit screen_producer(int x, int y, size_t width, size_t height)
		: head_(draw_frame::empty())
		, x_(x)
		, y_(y)
		, width_(width)
		, height_(height)
	{					
		frame_buffer_.push(draw_frame::empty());
		executor_.start();

		screen_ = ::CreateDC(L"DISPLAY", 0, 0, 0);
		mem_ = ::CreateCompatibleDC(screen_);
				
		BITMAPINFO info;
		memset(&info, 0, sizeof(BITMAPINFO));
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biHeight = -static_cast<int>(height_);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof(BITMAPINFO);
		info.bmiHeader.biWidth = width_;

		bmp_ = ::CreateDIBSection(screen_, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), 0, 0);
		SelectObject(mem_, bmp_);
	}

	~screen_producer()
	{
		::DeleteObject(bmp_);
		::DeleteDC(mem_);
		::DeleteDC(screen_);
	}

	virtual void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		frame_factory_ = frame_factory;
	}
	
	virtual safe_ptr<draw_frame> receive() 
	{
		if(frame_buffer_.try_pop(head_))
		{
			executor_.begin_invoke([=]
			{				
				auto frame = frame_factory_->create_frame(width_, height_);

				::BitBlt(mem_, 0, 0, width_, height_, screen_, x_, y_, SRCCOPY | CAPTUREBLT);	
	
				std::copy_n(bmp_data_, width_*height_*4, frame->image_data().begin());
				for(size_t i = 0; i< height_*width_; ++i)		
					frame->image_data().begin()[i*4+3] = 255;
				
				frame_buffer_.push(frame);
			});
		}

		return head_; 
	}
	
	virtual std::wstring print() const { return + L"screen[]"; }
};

safe_ptr<frame_producer> create_screen_producer(const std::vector<std::wstring>& params)
{
	if(params.size() < 5 || !boost::iequals(params[0], "screen"))
		return frame_producer::empty();

	int x = 0;
	int y = 0;
	size_t width = 0;
	size_t height = 0;
	
	try{x = boost::lexical_cast<int>(params[1]);}
	catch(boost::bad_lexical_cast&){return frame_producer::empty();}
	try{y = boost::lexical_cast<int>(params[2]);}
	catch(boost::bad_lexical_cast&){return frame_producer::empty();}
	try{width = boost::lexical_cast<size_t>(params[3]);}
	catch(boost::bad_lexical_cast&){return frame_producer::empty();}
	try{height = boost::lexical_cast<size_t>(params[4]);}
	catch(boost::bad_lexical_cast&){return frame_producer::empty();}

	return make_safe<screen_producer>(x, y, width, height);
}

}}