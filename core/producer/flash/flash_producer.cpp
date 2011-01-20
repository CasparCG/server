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

#include "flash_producer.h"
#include "FlashAxContainer.h"

#include <core/video_format.h>

#include <mixer/frame/draw_frame.h>
#include <mixer/frame_mixer_device.h>

#include <common/concurrency/executor.h>
#include <common/utility/timer.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace core { namespace flash {

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

class flash_renderer
{
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;

	const std::wstring filename_;
	const video_format_desc format_desc_;

	std::shared_ptr<frame_factory> frame_factory_;
	
	BYTE* bmp_data_;	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;

	CComObject<caspar::flash::FlashAxContainer>* ax_;
	safe_ptr<draw_frame> head_;

public:
	flash_renderer(const std::shared_ptr<frame_factory>& frame_factory, const std::wstring& filename) 
		: filename_(filename)
		, format_desc_(frame_factory->get_video_format_desc())
		, frame_factory_(frame_factory)
		, bmp_data_(nullptr)
		, hdc_(CreateCompatibleDC(0), DeleteDC)
		, ax_(nullptr)
		, head_(draw_frame::empty())
	{
		CASPAR_LOG(info) << print() << L" Started";
		
		if(FAILED(CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&ax_)))
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
														
		ax_->SetFormat(format_desc_);
		
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
	}

	~flash_renderer()
	{		
		if(ax_)
		{
			ax_->DestroyAxControl();
			ax_->Release();
		}
		CASPAR_LOG(info) << print() << L" Ended";
	}
	
	void param(const std::wstring& param)
	{		
		if(!ax_->FlashCall(param))
			CASPAR_LOG(warning) << "Flash Function Call Failed. Param: " << param;
	}
	
	safe_ptr<draw_frame> render_frame()
	{
		if(ax_->IsEmpty())
			return draw_frame::empty();

		auto frame = render_simple_frame();
		if(ax_->GetFPS()/2.0 - format_desc_.fps >= 0.0)
			frame = draw_frame::interlace(frame, render_simple_frame(), format_desc_.mode);
		return frame;
	}
		
	timer timer_;
	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"] Render thread"; }
	std::string bprint() const{ return narrow(print()); }

private:

	safe_ptr<draw_frame> render_simple_frame()
	{
		timer_.tick(1.0/ax_->GetFPS()); // Tick doesnt work on nested timelines, force an actual sync

		ax_->Tick();

		if(ax_->InvalidRect())
		{			
			std::fill_n(bmp_data_, format_desc_.size, 0);
			ax_->DrawControl(static_cast<HDC>(hdc_.get()));
		
			auto frame = frame_factory_->create_frame();
			std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());
			head_ = frame;
		}		

		return head_;
	}
};

struct flash_producer::implementation
{	
	safe_ptr<draw_frame> tail_;
	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;
	executor executor_;

	std::shared_ptr<flash_renderer> renderer_;
	std::shared_ptr<frame_factory> frame_factory_;
	
	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"]"; }	
	std::wstring filename_;

	implementation(const std::wstring& filename) 
		: filename_(filename)
		, tail_(draw_frame::empty())
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));		
	}

	~implementation()
	{
		executor_.clear();
		executor_.invoke([=]
		{
			renderer_ = nullptr;
		});
	}
	
	virtual void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		frame_factory_ = frame_factory;
		frame_buffer_.set_capacity(static_cast<size_t>(frame_factory->get_video_format_desc().fps/2.0));
		while(frame_buffer_.try_push(draw_frame::empty())){}
		executor_.start();
	}

	virtual safe_ptr<draw_frame> receive()
	{		
		if(!frame_buffer_.try_pop(tail_))
			CASPAR_LOG(trace) << print() << " underflow";

		if(executor_.size() > 8) // Limit the call queue in-case of underflow
			return tail_;

		executor_.begin_invoke([=]
		{
			if(!renderer_)
				return;

			try
			{
				auto frame = draw_frame::empty();
				do{frame = renderer_->render_frame();}
				while(frame_buffer_.try_push(frame) && frame == draw_frame::empty());
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				renderer_ = nullptr;
			}
		});	

		return tail_;
	}
	
	void param(const std::wstring& param) 
	{	
		executor_.begin_invoke([=]
		{
			if(!renderer_)
				renderer_.reset(new flash_renderer(frame_factory_, filename_));

			try
			{
				renderer_->param(param);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				renderer_ = nullptr;
			}
		});
	}
};

flash_producer::flash_producer(flash_producer&& other) : impl_(std::move(other.impl_)){}
flash_producer::flash_producer(const std::wstring& filename) : impl_(new implementation(filename)){}
safe_ptr<draw_frame> flash_producer::receive(){return impl_->receive();}
void flash_producer::param(const std::wstring& param){impl_->param(param);}
void flash_producer::initialize(const safe_ptr<frame_factory>& frame_factory) { impl_->initialize(frame_factory);}
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