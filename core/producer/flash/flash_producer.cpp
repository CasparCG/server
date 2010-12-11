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
#include "../../../common/utility/scope_exit.h"

#include "../../processor/producer_frame.h"
#include "../../processor/composite_frame.h"

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <type_traits>

namespace caspar { namespace core { namespace flash {

using namespace boost::assign;

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

struct flash_producer::implementation
{	
	implementation(flash_producer* self, const std::wstring& filename) 
		: flashax_container_(nullptr), filename_(filename), self_(self), executor_([=]{run();}), invalid_count_(0)
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(common::narrow(filename)));

		frame_buffer_.set_capacity(3);		
	}

	~implementation() 
	{
		stop();
	}

	void start(bool force = true)
	{		
		if(executor_.is_running() && !force)
			return;

		try
		{
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
			frame_buffer_.clear();
			executor_.stop();
		}
	}

	void param(const std::wstring& param) 
	{	
		if(!executor_.is_running())
		{
			try
			{
				start();
			}
			catch(caspar_exception& e)
			{
				e << msg_info("Flashproducer failed to recover from failure.");
				throw e;
			}
		}

		executor_.invoke([&]()
		{			
			for(size_t retries = 0; !flashax_container_->CallFunction(param); ++retries)
			{
				CASPAR_LOG(debug) << "Retrying. Count: " << retries;
				if(retries > 3)
					BOOST_THROW_EXCEPTION(operation_failed() << arg_name_info("param") << arg_value_info(common::narrow(param)));
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
				if(flashax_container_ != nullptr)
				{
					flashax_container_->DestroyAxControl();
					flashax_container_->Release();
					flashax_container_ = nullptr;
				}
			});

			CASPAR_SCOPE_EXIT([=]
			{						
				stop();

				frame_buffer_.clear();
				frame_buffer_.try_push(producer_frame::eof()); // EOF
		
				current_frame_ = nullptr;
			});

			while(executor_.is_running())
			{	
				if(!is_empty_)
				{
					render();	
					while(executor_.try_execute()){}
				}
				else
				{
					executor_.execute();
					while(executor_.try_execute()){}
				}
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		CASPAR_LOG(info) << L"Ended flash_producer Thread";
	}

	void render()
	{
		assert(flashax_container_);
		
		if(!is_empty_ || current_frame_ == nullptr)
		{
			if(!flashax_container_->IsReadyToRender())
			{
				CASPAR_LOG(trace) << "Flash Producer Underflow";
				boost::thread::yield();
				return;
			}

			auto format_desc = frame_processor_->get_video_format_desc();
			bool is_progressive = format_desc.mode == video_mode::progressive || (flashax_container_->GetFPS() - format_desc.fps/2 == 0);

			producer_frame result;

			if(is_progressive)							
				result = do_receive();		
			else
				result = composite_frame::interlace(do_receive(), do_receive(), format_desc.mode);
			
			frame_buffer_.push(result);
			is_empty_ = flashax_container_->IsEmpty();
		}
	}
		
	producer_frame do_receive()
	{
		auto format_desc = frame_processor_->get_video_format_desc();

		flashax_container_->Tick();
		invalid_count_ = !flashax_container_->InvalidRectangle() ? std::min(2, invalid_count_+1) : 0;
		if(current_frame_ == nullptr || invalid_count_ < 2)
		{				
			std::fill_n(bmp_frame_->data(), bmp_frame_->size(), 0);			
			flashax_container_->DrawControl(bmp_frame_->hdc());
			current_frame_ = bmp_frame_;
		}	

		auto frame = frame_processor_->create_frame(format_desc.width, format_desc.height);
		std::copy(current_frame_->data(), current_frame_->data() + current_frame_->size(), frame.pixel_data().begin());
		return producer_frame(std::make_shared<write_frame>(std::move(frame)));
	}
		
	producer_frame receive()
	{		
		return (frame_buffer_.try_pop(last_frame_) || !is_empty_) ? last_frame_ : producer_frame::empty();
	}

	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		frame_processor_ = frame_processor;
		auto format_desc = frame_processor_->get_video_format_desc();
		bmp_frame_ = std::make_shared<bitmap>(format_desc.width, format_desc.height);
		start(false);
	}

	std::wstring print() const
	{
		return L"flash_producer. filename: " + filename_;
	}
	
	CComObject<flash::FlashAxContainer>* flashax_container_;
		
	tbb::concurrent_bounded_queue<producer_frame> frame_buffer_;

	producer_frame last_frame_;

	bitmap_ptr current_frame_;
	bitmap_ptr bmp_frame_;

	std::wstring filename_;
	flash_producer* self_;

	tbb::atomic<bool> is_empty_;
	common::executor executor_;
	int invalid_count_;

	frame_processor_device_ptr frame_processor_;
};

flash_producer::flash_producer(const std::wstring& filename) : impl_(new implementation(this, filename)){}
producer_frame flash_producer::receive(){return impl_->receive();}
void flash_producer::param(const std::wstring& param){impl_->param(param);}
void flash_producer::initialize(const frame_processor_device_ptr& frame_processor) { impl_->initialize(frame_processor);}
std::wstring flash_producer::print() const {return impl_->print();}

std::wstring flash_producer::find_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + L".ft")) 
		return template_name + L".ft";
	
	if(boost::filesystem::exists(template_name + L".ct"))
		return template_name + L".ct";

	return L"";
}

flash_producer_ptr create_flash_producer(const std::vector<std::wstring>& params)
{
	static const std::vector<std::wstring> extensions = list_of(L"swf");
	std::wstring filename = server::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return nullptr;

	return std::make_shared<flash_producer>(filename + L"." + *ext);
}

}}}