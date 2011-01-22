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
 
#include "..\..\stdafx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#pragma warning (disable : 4146)
#endif

#include "silverlight_producer.h"

#include "XcpControlHost.h"

#include <core/video_format.h>

#include <mixer/frame/draw_frame.h>
#include <mixer/frame_factory.h>

#include <common/concurrency/executor.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace core {
	
class silverlight_renderer
{
	tbb::atomic<bool> is_running_;
	std::wstring filename_;
	std::shared_ptr<frame_factory> frame_factory_;
	video_format_desc format_desc_;
	
	unsigned char* bmp_data_;
	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;

	CComObject<XcpControlHost>* host_;
	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;	
	safe_ptr<draw_frame> last_frame_;

	tbb::concurrent_queue<std::wstring> params_;
public:
	silverlight_renderer() 
		: last_frame_(draw_frame::empty())
		, bmp_data_(nullptr)
		, host_(nullptr) {}

	~silverlight_renderer()
	{		
		if(host_)
		{
			host_->DestroyXcpControl();
			host_->Release();
		}
	}

	void load(const std::shared_ptr<frame_factory>& frame_factory)
	{
		format_desc_ = frame_factory->get_video_format_desc();
		hdc_.reset(CreateCompatibleDC(0), DeleteDC);
		frame_factory_ = frame_factory;
		
		if(FAILED(CComObject<XcpControlHost>::CreateInstance(&host_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create XcpControlHost"));
		
		if(FAILED(host_->CreateXcpControl()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Create XcpControl"));
		
		host_->SetSize(format_desc_.width, format_desc_.height);

		BITMAPINFO info;
		memset(&info, 0, sizeof(BITMAPINFO));
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biHeight = -format_desc_.height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof(BITMAPINFO);
		info.bmiHeader.biWidth = format_desc_.width;

		bmp_.reset(CreateDIBSection(static_cast<HDC>(hdc_.get()), &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), 0, 0), DeleteObject);
		SelectObject(static_cast<HDC>(hdc_.get()), bmp_.get());	

		frame_buffer_.set_capacity(20);
		for(int n = 0; n < frame_buffer_.capacity()/2; ++n)
			frame_buffer_.try_push(draw_frame::empty());
		is_running_ = true;
	}
		
	void render()
	{	
		std::fill_n(bmp_data_, format_desc_.size, 0);
		host_->DrawControl(static_cast<HDC>(hdc_.get()));
		
		auto frame = frame_factory_->create_frame();
		std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());
					
		frame_buffer_.try_push(frame);
	}
		
	safe_ptr<draw_frame> get_frame()
	{		
		frame_buffer_.try_pop(last_frame_);
		return last_frame_;
	}
	
	void stop()
	{
		is_running_ = false;
	}
};

struct silverlight_producer : public frame_producer
{		
	boost::thread thread_;
	tbb::atomic<DWORD> id_;

	std::shared_ptr<silverlight_renderer> renderer_;
	std::shared_ptr<frame_factory> frame_factory_;
	
	std::wstring print() const{ return L"silverlight[]"; }	
public:
	silverlight_producer() 
		: renderer_(std::make_shared<silverlight_renderer>())
	{	
		id_ = 0;
	}

	~silverlight_producer()
	{
		renderer_->stop();
		thread_.join();
	}
		
	void run()
	{
		CASPAR_LOG(info) << print() << " started.";

		::OleInitialize(nullptr);
		
		renderer_->load(frame_factory_);

		id_ = GetCurrentThreadId();

		MSG msg;
		while(GetMessage(&msg, 0, 0, 0))
		{		
			if(msg.message == WM_USER + 1)
				renderer_->render();

			TranslateMessage(&msg);
			DispatchMessage(&msg);	
		}

		renderer_ = nullptr;

		::OleUninitialize();

		CASPAR_LOG(info) << print() << " ended.";
	}

	virtual void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		frame_factory_ = frame_factory;
		thread_ = boost::thread([this]{run();});
	}

	virtual safe_ptr<draw_frame> receive()
	{
		if(id_ == 0)
			return draw_frame::empty();

		PostThreadMessage(id_, WM_USER + 1, 0 ,0);
		return renderer_->get_frame();
	}
};

safe_ptr<frame_producer> create_silverlight_producer(const std::vector<std::wstring>& params)
{
	//std::wstring filename = params[0] + L".xap";
	//if(!boost::filesystem::exists(filename))
	//	return frame_producer::empty();
	if(params[0] != L"SILVER")
		return frame_producer::empty();

	return make_safe<silverlight_producer>();
}

}}