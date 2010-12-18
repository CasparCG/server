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
#include "bitmap.h"

#include "../../format/video_format.h"
#include "../../server.h"
#include "../../../common/concurrency/executor.h"
#include "../../../common/concurrency/concurrent_queue.h"
#include "../../../common/utility/scope_exit.h"

#include "../../processor/draw_frame.h"
#include "../../processor/composite_frame.h"

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <tbb/atomic.h>

#include <type_traits>

namespace caspar { namespace core { namespace flash {

using namespace boost::assign;

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

struct flash_producer::implementation
{	
	implementation(flash_producer* self, const std::wstring& filename) 
		: flashax_container_(nullptr), filename_(filename), self_(self), executor_([=]{run();}), invalid_count_(0), last_frame_(draw_frame::empty())
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));

		frame_buffer_.set_capacity(3);
	}

	~implementation() 
	{
		stop();
	}

	void start()
	{		
		try
		{
			safe_ptr<draw_frame> frame = draw_frame::eof();
			while(frame_buffer_.try_pop(frame)){}

			is_empty_ = true;
			executor_.stop(); // Restart if running
			executor_.start();
			executor_.invoke([=]()
			{
				if(FAILED(CComObject<FlashAxContainer>::CreateInstance(&flashax_container_)) || 
							flashax_container_ == nullptr)
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create FlashAxContainer"));
		
				flashax_container_->pflash_producer_ = self_;
				CComPtr<IShockwaveFlash> spFlash;

				if(FAILED(flashax_container_->CreateAxControl()))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Create FlashAxControl"));

				if(FAILED(flashax_container_->QueryControl(&spFlash)))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Query FlashAxControl"));
												
				if(FAILED(spFlash->put_Playing(true)) )
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to start playing Flash"));

				if(FAILED(spFlash->put_Movie(CComBSTR(filename_.c_str()))))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Load Template Host"));
						
				//Exact fit. Scale without respect to the aspect ratio.
				if(FAILED(spFlash->put_ScaleMode(2))) 
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Set Scale Mode"));
												
				// stop if failed
				if(FAILED(flashax_container_->SetFormat(frame_processor_->get_video_format_desc()))) 
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to Set Format"));

				current_frame_ = nullptr; // Force re-render of current frame	
				last_frame_ = draw_frame::empty();
			});
		}
		catch(...)
		{
			stop();
			CASPAR_LOG_CURRENT_EXCEPTION();
			throw;
		}
	}

	void stop()
	{
		is_empty_ = true;
		if(executor_.is_running())
		{
			safe_ptr<draw_frame> frame = draw_frame::eof();
			while(frame_buffer_.try_pop(frame)){}
			executor_.stop();
		}
	}

	void param(const std::wstring& param) 
	{	
		if(!executor_.is_running())
			start();

		executor_.invoke([&]()
		{			
			for(size_t retries = 0; !flashax_container_->CallFunction(param); ++retries)
			{
				CASPAR_LOG(debug) << "Retrying. Count: " << retries;
				if(retries > 3)
					BOOST_THROW_EXCEPTION(operation_failed() << arg_name_info("param") << arg_value_info(narrow(param)));
			}
			is_empty_ = false;	
		});
	}
	
	void run()
	{
		win32_exception::install_handler();
		CASPAR_LOG(info) << L"started flash_producer Thread";

		try
		{
			::OleInitialize(nullptr);
			CASPAR_SCOPE_EXIT(::OleUninitialize);

			CASPAR_SCOPE_EXIT([=]
			{
				if(flashax_container_)
				{
					flashax_container_->DestroyAxControl();
					flashax_container_->Release();
					flashax_container_ = nullptr;
				}
			});
			
			while(executor_.is_running())
			{	
				if(!is_empty_)
				{
					render();	
					while(executor_.try_execute()){}
				}
				else				
					executor_.execute();				
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		frame_buffer_.try_push(draw_frame::eof()); // EOF
		CASPAR_LOG(info) << L"Ended flash_producer Thread";
	}

	void render()
	{		
		if(!is_empty_ || current_frame_ == nullptr)
		{
			bool is_progressive = format_desc_.mode == video_mode::progressive || (flashax_container_->GetFPS() - format_desc_.fps/2 == 0);

			safe_ptr<draw_frame> frame = render_frame();
			if(!is_progressive)
				frame = composite_frame::interlace(frame, render_frame(), format_desc_.mode);
			
			frame_buffer_.push(std::move(frame));
			is_empty_ = flashax_container_->IsEmpty();
		}
	}

	safe_ptr<draw_frame> render_frame()
	{
		if(!flashax_container_->IsReadyToRender())
			return draw_frame::empty();

		flashax_container_->Tick();
		invalid_count_ = !flashax_container_->InvalidRectangle() ? std::min(2, invalid_count_+1) : 0;
		if(current_frame_ == nullptr || invalid_count_ < 2)
		{
			std::fill_n(bmp_frame_->data(), bmp_frame_->size(), 0);			
			flashax_container_->DrawControl(bmp_frame_->hdc());
			current_frame_ = bmp_frame_;
		}

		auto frame = frame_processor_->create_frame(format_desc_.width, format_desc_.height);
		std::copy(current_frame_->data(), current_frame_->data() + current_frame_->size(), frame->pixel_data().begin());
		return frame;
	}

	safe_ptr<draw_frame> receive()
	{
		return frame_buffer_.try_pop(last_frame_) || !is_empty_ ? last_frame_ : draw_frame::empty();
	}

	void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		frame_processor_ = frame_processor;
		format_desc_ = frame_processor_->get_video_format_desc();
		bmp_frame_ = std::make_shared<bitmap>(format_desc_.width, format_desc_.height);
		start();
	}

	std::wstring print() const
	{
		return L"flash_producer. filename: " + filename_;
	}
	
	CComObject<flash::FlashAxContainer>* flashax_container_;

	concurrent_bounded_queue_r<safe_ptr<draw_frame>> frame_buffer_;

	safe_ptr<draw_frame> last_frame_;

	bitmap_ptr current_frame_;
	bitmap_ptr bmp_frame_;

	std::wstring filename_;
	flash_producer* self_;

	tbb::atomic<bool> is_empty_;
	executor executor_;
	int invalid_count_;

	std::shared_ptr<frame_processor_device> frame_processor_;

	video_format_desc format_desc_;
};

flash_producer::flash_producer(flash_producer&& other) : impl_(std::move(other.impl_)){}
flash_producer::flash_producer(const std::wstring& filename) : impl_(new implementation(this, filename)){}
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

safe_ptr<flash_producer> create_flash_producer(const std::vector<std::wstring>& params)
{
	static const std::vector<std::wstring> extensions = list_of(L"swf");
	std::wstring filename = server::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info(narrow(filename)));

	return make_safe<flash_producer>(filename + L"." + *ext);
}

}}}