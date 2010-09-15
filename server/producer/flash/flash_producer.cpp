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

#include "../../frame/bitmap_frame.h"
#include "../../frame/frame_format.h"
#include "../../frame/system_frame.h"
#include "../../../common/utility/find_file.h"
#include "../../server.h"
#include "../../../common/concurrency/executor.h"
#include "../../../common/concurrency/function_task.h"
#include "../../../common/image/image.h"
#include "../../../common/utility/scope_exit.h"

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <type_traits>

namespace caspar { namespace flash {

using namespace boost::assign;

// This is needed in order to make CComObject work since this is not a real ATL project
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

struct flash_producer::implementation
{	
	implementation(flash_producer* self, const std::wstring& filename, const frame_format_desc& format_desc, Monitor* monitor) 
		: flashax_container_(nullptr), filename_(filename), self_(self), format_desc_(format_desc), monitor_(monitor),
			bitmap_pool_(new bitmap_pool), executor_([=]{run();})
	{	
    	if(!boost::filesystem::exists(filename))
    		BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(common::narrow(filename)));

		frame_buffer_.set_capacity(flash_producer::DEFAULT_BUFFER_SIZE);
		last_frame_ = std::make_shared<bitmap_frame>(format_desc_.width, format_desc_.height);

		start();
	}

	~implementation() 
	{
		stop();
	}

	void start()
	{		
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
				if(FAILED(flashax_container_->SetFormat(format_desc_))) 
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
					BOOST_THROW_EXCEPTION(operation_failed() << warg_name_info(L"param") << warg_value_info(param));
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
				frame_buffer_.try_push(nullptr); // EOF
		
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

			bool isProgressive = format_desc_.mode == video_mode::progressive || (flashax_container_->GetFPS() - format_desc_.fps/2 == 0);
			frame_buffer_.push(isProgressive ? render_frame() : render_interlace_frame());
			is_empty_ = flashax_container_->IsEmpty();
		}
	}

	frame_ptr render_interlace_frame()
	{		
		return copy_frame(render_frame(), render_frame(), format_desc_);
	}

	frame_ptr render_frame()
	{
		flashax_container_->Tick();
		if(current_frame_ == nullptr || flashax_container_->InvalidRectangle())
		{		
			bitmap_frame_ptr frame;		
			if(!bitmap_pool_->try_pop(frame))					
			{	
				CASPAR_LOG(trace) << "Allocated bitmap_frame";
				frame = clear_frame(std::make_shared<bitmap_frame>(format_desc_.width, format_desc_.height));			
			}
			flashax_container_->DrawControl(frame->hdc());

			auto pool = bitmap_pool_;
			current_frame_.reset(frame.get(), [=](bitmap_frame*)
			{
				common::function_task::enqueue([=]
				{
					try
					{
						pool->try_push(clear_frame(frame));
					}
					catch(...){}
				});
			});
		}	
		return current_frame_;
	}
		
	frame_ptr get_frame()
	{
		return frame_buffer_.try_pop(last_frame_) || !is_empty_ ? last_frame_ : frame::null();
	}
	
	typedef tbb::concurrent_bounded_queue<bitmap_frame_ptr> bitmap_pool;
	std::shared_ptr<bitmap_pool> bitmap_pool_;
	frame_format_desc format_desc_;

	CComObject<caspar::flash::FlashAxContainer>* flashax_container_;
		
	tbb::concurrent_bounded_queue<frame_ptr> frame_buffer_;
	frame_ptr last_frame_;
	frame_ptr current_frame_;
	
	std::wstring filename_;
	flash_producer* self_;
	Monitor* monitor_;

	tbb::atomic<bool> is_empty_;
	common::executor executor_;
};

flash_producer::flash_producer(const std::wstring& filename, const frame_format_desc& format_desc, Monitor* monitor) : impl_(new implementation(this, filename, format_desc, monitor)){}
frame_ptr flash_producer::get_frame(){return impl_->get_frame();}
Monitor* flash_producer::get_monitor(){return impl_->monitor_; }
void flash_producer::param(const std::wstring& param){impl_->param(param);}
const frame_format_desc& flash_producer::get_frame_format_desc() const { return impl_->format_desc_; } 

std::wstring flash_producer::find_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + TEXT(".ft"))) 
		return template_name + TEXT(".ft");
	
	if(boost::filesystem::exists(template_name + TEXT(".ct")))
		return template_name + TEXT(".ct");

	return TEXT("");
}

flash_producer_ptr create_flash_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	// TODO: Check for flash support
	auto filename = params[0];
	std::wstring result_filename = common::find_file(server::media_folder() + filename, 
											list_of(L"swf"));

	return result_filename.empty() ? nullptr : std::make_shared<flash_producer>(result_filename, format_desc);
}

}}