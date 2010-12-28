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
#endif

#include "flash_producer.h"
#include "FlashAxContainer.h"
#include "TimerHelper.h"

#include "../../format/video_format.h"

#include "../../processor/draw_frame.h"

#include <common/concurrency/executor.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace core { namespace flash {

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

class flash_renderer
{
public:
	flash_renderer(const safe_ptr<frame_processor_device>& frame_processor, const std::wstring& filename) 
		: last_frame_(draw_frame::empty()), current_frame_(draw_frame::empty()), frame_processor_(frame_processor), filename_(filename),
		hdc_(CreateCompatibleDC(0), DeleteDC), format_desc_(frame_processor->get_video_format_desc())

	{
		CASPAR_LOG(info) << print() << L" Started";

		::OleInitialize(nullptr);

		CComObject<FlashAxContainer>* object;		
		if(FAILED(CComObject<FlashAxContainer>::CreateInstance(&object)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(bprint() + "Failed to create FlashAxContainer"));
		
		object->AddRef();
		ax_.Attach(object);
		
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

		BITMAPINFO info;
		memset(&info, 0, sizeof(BITMAPINFO));
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
#ifdef _MSC_VER
	#pragma warning(disable:4146)
#endif
		info.bmiHeader.biHeight = -format_desc_.height;
#ifdef _MSC_VER
	#pragma warning(default:4146)
#endif
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof(BITMAPINFO);
		info.bmiHeader.biWidth = format_desc_.width;

		bmp_.reset(CreateDIBSection((HDC)hdc_.get(), &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), 0, 0), DeleteObject);
		SelectObject((HDC)hdc_.get(), bmp_.get());	
	}

	~flash_renderer()
	{
		try
		{
			ax_->DestroyAxControl();
			ax_.Release();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		::OleUninitialize();
		CASPAR_LOG(info) << print() << L" Ended";
	}

	void param(const std::wstring& param) 
	{			
		for(size_t retries = 0; !ax_->CallFunction(param); ++retries)
		{
			CASPAR_LOG(debug) << print() << L" Retrying. Count: " << retries;
			if(retries > 3)
				BOOST_THROW_EXCEPTION(operation_failed() << arg_name_info("param") << arg_value_info(narrow(param)) << msg_info(narrow(print())));
		}
	}
		
	void render()
	{		 
		bool is_progressive = format_desc_.mode == video_mode::progressive || (ax_->GetFPS() - format_desc_.fps/2 == 0);
		
		safe_ptr<draw_frame> frame = render_frame();
		if(!is_progressive)
			frame = draw_frame::interlace(frame, render_frame(), format_desc_.mode);
			
		frame_buffer_.try_push(std::move(frame));
	}

	safe_ptr<draw_frame> render_frame()
	{
		if(!ax_->IsEmpty())
			ax_->Tick();
		
		if(ax_->IsReadyToRender() && ax_->InvalidRectangle())
		{
			std::fill_n(bmp_data_, format_desc_.size, 0);			
			//ax_->DrawControl((HDC)hdc_.get());
		
			auto frame = frame_processor_->create_frame();
			std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());
			current_frame_ = frame;
		}
		return current_frame_;
	}
	
	bool try_pop(safe_ptr<draw_frame>& dest)
	{
		std::shared_ptr<draw_frame> temp;
		bool result = frame_buffer_.try_pop(temp);
		if(temp)
			last_frame_ = safe_ptr<draw_frame>(std::move(temp));
		dest = last_frame_;
		return result;
	}

	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"] Render thread"; }
	std::string bprint() const{ return narrow(print()); }

private:
	const std::wstring filename_;
	const safe_ptr<frame_processor_device> frame_processor_;
	const video_format_desc format_desc_;
	
	unsigned char* bmp_data_;
	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;

	CComPtr<FlashAxContainer> ax_;
	tbb::concurrent_bounded_queue<std::shared_ptr<draw_frame>> frame_buffer_;	
	safe_ptr<draw_frame> last_frame_;
	safe_ptr<draw_frame> current_frame_;
};

struct flash_producer::implementation
{	
	implementation(const std::wstring& filename) : filename_(filename)
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));

		executor_.start();
	}

	~implementation() 
	{
		executor_.invoke([this]{renderer_.reset();});
	}
	
	void param(const std::wstring& param) 
	{	
		if(!factory_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info(narrow(print()) + "Uninitialized."));

		executor_.invoke([&]
		{
			if(!renderer_)
			{
				renderer_.reset(factory_());
				for(int n = 0; n < 8; ++n)
					render_frame();
			}
			renderer_->param(param);
		});
	}
	
	virtual safe_ptr<draw_frame> receive()
	{
		auto frame = draw_frame::empty();
		if(renderer_ && renderer_->try_pop(frame)) // Only render again if frame was removed from buffer.		
			executor_.begin_invoke([this]{render_frame();});	
		else
			CASPAR_LOG(trace) << print() << " underflow.";
		return frame;
	}

	virtual void render_frame()
	{
		try
		{
			renderer_->render();
		}
		catch(...)
		{
			renderer_.reset();
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	virtual void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		factory_ = [=]{return new flash_renderer(frame_processor, filename_);};
	}
	
	std::wstring print() const{ return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"]"; }
	
	std::wstring filename_;
	std::function<flash_renderer*()> factory_;

	std::unique_ptr<flash_renderer> renderer_;
	executor executor_;
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