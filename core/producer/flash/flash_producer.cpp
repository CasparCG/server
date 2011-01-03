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
#pragma warning(disable:4146)
#endif

#include "flash_producer.h"
#include "FlashAxContainer.h"

#include "../../format/video_format.h"

#include "../../processor/draw_frame.h"
#include "../../processor/frame_processor_device.h"

#include <common/concurrency/executor.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace core { namespace flash {

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

class flash_renderer
{
public:
	flash_renderer() : tail_(draw_frame::empty()), head_(draw_frame::empty()), bmp_data_(nullptr), ax_(nullptr) {}

	~flash_renderer()
	{		
		if(ax_)
		{
			ax_->DestroyAxControl();
			ax_->Release();
		}
		CASPAR_LOG(info) << print() << L" Ended";
	}

	void load(const std::shared_ptr<frame_processor_device>& frame_processor, const std::wstring& filename)
	{
		filename_ = filename;
		hdc_.reset(CreateCompatibleDC(0), DeleteDC);
		frame_processor_ = frame_processor;
		CASPAR_LOG(info) << print() << L" Started";
		
		if(FAILED(CComObject<FlashAxContainer>::CreateInstance(&ax_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to create FlashAxContainer"));
		
		if(FAILED(ax_->CreateAxControl()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to Create FlashAxControl"));
		
		CComPtr<IShockwaveFlash> spFlash;
		if(FAILED(ax_->QueryControl(&spFlash)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to Query FlashAxControl"));
												
		if(FAILED(spFlash->put_Playing(true)) )
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to start playing Flash"));

		if(FAILED(spFlash->put_Movie(CComBSTR(filename.c_str()))))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to Load Template Host"));
										
		if(FAILED(spFlash->put_ScaleMode(2)))  //Exact fit. Scale without respect to the aspect ratio.
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to Set Scale Mode"));
														
		if(FAILED(ax_->SetFormat(frame_processor_->get_video_format_desc())))  // stop if failed
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to Set Format"));
		
		format_desc_ = frame_processor->get_video_format_desc();

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

	void param(const std::wstring& param) 
	{		
		params_.push(param);
	}
		
	void tick()
	{		
		std::wstring param;
		if(params_.try_pop(param))
		{
			for(size_t retries = 0; !ax_->CallFunction(param); ++retries)
			{
				CASPAR_LOG(debug) << print() << L" Retrying. Count: " << retries;
				if(retries > 3)
					CASPAR_LOG(warning) << "Flash Function Call Failed. Param: " << param;
			}
		}	

		if(!is_running_)
			PostQuitMessage(0);
	}

	void render()
	{
		if(ax_->IsReadyToRender() && ax_->InvalidRectangle())
		{			
			std::fill_n(bmp_data_, format_desc_.size, 0);
			ax_->DrawControl(static_cast<HDC>(hdc_.get()));
		
			auto frame = frame_processor_->create_frame();
			std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());
			head_ = frame;
		}				
		if(!frame_buffer_.try_push(head_))
			CASPAR_LOG(trace) << print() << " overflow";
	}
		
	safe_ptr<draw_frame> get_frame()
	{		
		if(!frame_buffer_.try_pop(tail_))
			CASPAR_LOG(trace) << print() << " underflow";

		auto frame = tail_;
		if(frame_buffer_.size() > frame_buffer_.capacity()/2) // Regulate between interlaced and progressive
		{
			frame_buffer_.try_pop(tail_);
			frame = draw_frame::interlace(frame, tail_, format_desc_.mode);
		}
		return frame;
	}
	
	void stop()
	{
		is_running_ = false;
	}

	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"] Render thread"; }
	std::string bprint() const{ return narrow(print()); }

private:
	tbb::atomic<bool> is_running_;
	std::wstring filename_;
	std::shared_ptr<frame_processor_device> frame_processor_;
	video_format_desc format_desc_;
	
	unsigned char* bmp_data_;
	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;

	CComObject<FlashAxContainer>* ax_;
	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;	
	safe_ptr<draw_frame> tail_;
	safe_ptr<draw_frame> head_;

	tbb::concurrent_queue<std::wstring> params_;
};

struct flash_producer::implementation
{	
	implementation(const std::wstring& filename) : filename_(filename), renderer_(std::make_shared<flash_renderer>())
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));
	}

	~implementation()
	{
		renderer_->stop();
	}
	
	void param(const std::wstring& param) 
	{	
		renderer_->param(param);
	}
	
	void run()
	{
		CASPAR_LOG(info) << print() << " started.";

		::OleInitialize(nullptr);
		
		volatile auto keep_alive = renderer_;
		renderer_->load(frame_processor_, filename_);

		MSG msg;
		while(GetMessage(&msg, 0, 0, 0))
		{		
			if(msg.message == WM_USER + 1)
				renderer_->render();

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			renderer_->tick();		
		}

		renderer_ = nullptr;

		::OleUninitialize();

		CASPAR_LOG(info) << print() << " ended.";
	}

	virtual void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		frame_processor_ = frame_processor;
		thread_ = boost::thread([this]{run();});
	}

	virtual safe_ptr<draw_frame> receive()
	{
		return renderer_->get_frame();
	}
	
	boost::thread thread_;

	std::shared_ptr<flash_renderer> renderer_;
	std::shared_ptr<frame_processor_device> frame_processor_;
	
	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"]"; }	
	std::wstring filename_;
};

flash_producer::flash_producer(flash_producer&& other) : impl_(std::move(other.impl_)){}
flash_producer::flash_producer(const std::wstring& filename) : impl_(new implementation(filename)){}
safe_ptr<draw_frame> flash_producer::receive(){return impl_->receive();}
void flash_producer::param(const std::wstring& param){impl_->param(param);}
void flash_producer::initialize(const safe_ptr<frame_processor_device>& frame_processor) { impl_->initialize(frame_processor);}
std::wstring flash_producer::print() const {return impl_->print();}

std::wstring flash_producer::find_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + L".ft")) 
		return template_name + L".ft";
	
	if(boost::filesystem::exists(template_name + L".ct"))
		return template_name + L".ct";

	return L"";
}

}}}