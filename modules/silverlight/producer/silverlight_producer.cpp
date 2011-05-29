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
 
#include "../stdafx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#pragma warning (disable : 4146)
#endif

#include "silverlight_producer.h"

#include "../interop/XcpControlHost.h"

#include <core/video_format.h>

#include <core/mixer/write_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/concurrency/executor.h>
#include <common/env.h>

#include <boost/filesystem.hpp>

#include <SFML/Graphics.hpp>

namespace caspar {
		
class silverlight_renderer
{
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co;
	
	int id_;

	const std::shared_ptr<core::frame_factory> frame_factory_;
	const core::video_format_desc format_desc_;
	
	CComObject<XcpControlHost>* host_;
	tbb::spin_mutex mutex_;
	safe_ptr<core::basic_frame> last_frame_;

	sf::RenderWindow window_;
			
	HDC screen_;
	HDC mem_;
	HBITMAP bmp_;
	unsigned char* bmp_data_;

public:
	silverlight_renderer(const std::shared_ptr<core::frame_factory>& frame_factory) 
		: frame_factory_(frame_factory)
		, format_desc_(frame_factory->get_video_format_desc())
		, last_frame_(core::basic_frame::empty())
		, host_(nullptr)
		, id_(rand())
		, window_(sf::VideoMode(format_desc_.width, format_desc_.height, 32), boost::lexical_cast<std::string>(id_), sf::Style::None)
	{
		if(FAILED(CComObject<XcpControlHost>::CreateInstance(&host_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create XcpControlHost"));
		
		HWND hWnd = ::FindWindow(L"SFML_Window", boost::lexical_cast<std::wstring>(id_).c_str());
		if(FAILED(host_->CreateXcpControl(hWnd)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Create XcpControl"));
		
		screen_= ::GetDC(hWnd);
		mem_ = ::CreateCompatibleDC(screen_);
				
		BITMAPINFO info;
		memset(&info, 0, sizeof(BITMAPINFO));
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biHeight = -static_cast<int>(format_desc_.height);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof(BITMAPINFO);
		info.bmiHeader.biWidth = format_desc_.width;

		bmp_ = ::CreateDIBSection(screen_, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), 0, 0);
		SelectObject(mem_, bmp_);
	}

	~silverlight_renderer()
	{		
		DeleteObject(bmp_);
		DeleteDC(mem_);
		DeleteDC(screen_);

		if(host_)
		{
			host_->DestroyXcpControl();
			host_->Release();
		}
	}
			
	void render()
	{	
		sf::Event Event;
		while (window_.GetEvent(Event)){}
		window_.Display();
				
		auto frame = frame_factory_->create_frame(this, format_desc_.width, format_desc_.height);
		::BitBlt(mem_, 0, 0, format_desc_.width, format_desc_.height, screen_, 0, 0, SRCCOPY);		
		std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());

		tbb::spin_mutex::scoped_lock lock(mutex_);
		last_frame_ = frame;
	}
		
	safe_ptr<core::basic_frame> get_frame()
	{		
		tbb::spin_mutex::scoped_lock lock(mutex_);
		return last_frame_;
	}
};

struct silverlight_producer : public core::frame_producer
{				
	std::unique_ptr<silverlight_renderer> renderer_;
	
	executor executor_;
public:

	silverlight_producer(const safe_ptr<core::frame_factory>& frame_factory) : executor_(L"silverlight")
	{
		executor_.invoke([=]
		{
			renderer_.reset(new silverlight_renderer(frame_factory));
		});
	}
		
	virtual safe_ptr<core::basic_frame> receive()
	{
		executor_.begin_invoke([=]
		{
			renderer_->render();
		});

		return renderer_->get_frame();
	}

	std::wstring print() const{ return L"silverlight"; }	
};

safe_ptr<core::frame_producer> create_silverlight_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	//std::wstring filename = env::template_folder() + L"\\" + params[0] + L".xap";
	//if(!boost::filesystem::exists(filename))
	//	return frame_producer::empty();
	if(params[0] != L"SILVER")
		return core::frame_producer::empty();

	return make_safe<silverlight_producer>(frame_factory);
}

}