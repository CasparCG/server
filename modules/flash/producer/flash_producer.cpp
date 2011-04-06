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
#pragma warning (disable : 4146)
#endif

#include "flash_producer.h"
#include "FlashAxContainer.h"

#include <core/video_format.h>

#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/write_frame.h>

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/utility/timer.h>
#include <common/diagnostics/graph.h>

#include <boost/filesystem.hpp>

#include <functional>
#include <boost/thread.hpp>

namespace caspar {
		
bool interlaced(double fps, const core::video_format_desc& format_desc)
{
	return abs(fps/2.0 - format_desc.fps) < 0.1;
}

class flash_renderer
{
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;
	
	const std::wstring filename_;
	const core::video_format_desc format_desc_;

	std::shared_ptr<core::frame_factory> frame_factory_;
	
	BYTE* bmp_data_;	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;

	CComObject<caspar::flash::FlashAxContainer>* ax_;
	safe_ptr<core::basic_frame> head_;
	
	safe_ptr<diagnostics::graph> graph_;
	timer perf_timer_;
	
public:
	flash_renderer(const safe_ptr<diagnostics::graph>& graph, const std::shared_ptr<core::frame_factory>& frame_factory, const std::wstring& filename) 
		: graph_(graph)
		, filename_(filename)
		, format_desc_(frame_factory->get_video_format_desc())
		, frame_factory_(frame_factory)
		, bmp_data_(nullptr)
		, hdc_(CreateCompatibleDC(0), DeleteDC)
		, ax_(nullptr)
		, head_(core::basic_frame::empty())
	{
		graph_->add_guide("frame-time", 0.5f);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));	
		graph_->set_color("param", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));			
		
		if(FAILED(CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&ax_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to create FlashAxContainer"));
		
		ax_->set_print([this]{return L"";});

		if(FAILED(ax_->CreateAxControl()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to Create FlashAxControl"));
		
		CComPtr<IShockwaveFlash> spFlash;
		if(FAILED(ax_->QueryControl(&spFlash)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to Query FlashAxControl"));
												
		if(FAILED(spFlash->put_Playing(true)) )
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to start playing Flash"));

		if(FAILED(spFlash->put_Movie(CComBSTR(filename.c_str()))))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to Load Template Host"));
										
		if(FAILED(spFlash->put_ScaleMode(2)))  //Exact fit. Scale without respect to the aspect ratio.
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to Set Scale Mode"));
						
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
		CASPAR_LOG(info) << print() << L" Thread started.";
	}

	~flash_renderer()
	{		
		if(ax_)
		{
			ax_->DestroyAxControl();
			ax_->Release();
		}
		CASPAR_LOG(info) << print() << L" Thread ended.";
	}
	
	void param(const std::wstring& param)
	{		
		if(!ax_->FlashCall(param))
			CASPAR_LOG(warning) << print() << " Flash function call failed. Param: " << param << ".";
		graph_->add_tag("param");

		if(abs(ax_->GetFPS() - format_desc_.fps) > 0.001 && abs(ax_->GetFPS()/2.0 - format_desc_.fps) > 0.001)
			CASPAR_LOG(warning) << print() << " Invalid frame-rate: " << ax_->GetFPS() << L". Should be either " << format_desc_.fps << L" or " << format_desc_.fps*2.0 << L".";
	}
	
	safe_ptr<core::basic_frame> render_frame(bool has_underflow)
	{
		if(ax_->IsEmpty())
			return core::basic_frame::empty();

		auto frame = render_simple_frame(has_underflow);
		if(interlaced(ax_->GetFPS(), format_desc_))
			frame = core::basic_frame::interlace(frame, render_simple_frame(has_underflow), format_desc_.mode);
		return frame;
	}

	double fps() const
	{
		return ax_->GetFPS();	
	}
	
	std::wstring print()
	{
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"]";		
	}
	
private:

	safe_ptr<core::basic_frame> render_simple_frame(bool underflow)
	{
		if(underflow)
			graph_->add_tag("underflow");

		double frame_time = 1.0/ax_->GetFPS();

		perf_timer_.reset();
		ax_->Tick();

		if(ax_->InvalidRect())
		{			
			std::fill_n(bmp_data_, format_desc_.size, 0);
			ax_->DrawControl(static_cast<HDC>(hdc_.get()));
		
			auto frame = frame_factory_->create_frame();
			std::copy_n(bmp_data_, format_desc_.size, frame->image_data().begin());
			head_ = frame;
		}		
		
		graph_->update_value("frame-time", static_cast<float>(perf_timer_.elapsed()/frame_time));
		return head_;
	}
};

struct flash_producer : public core::frame_producer
{	
	const std::wstring filename_;	
	tbb::atomic<int> fps_;

	std::shared_ptr<diagnostics::graph> graph_;

	safe_ptr<core::basic_frame> tail_;
	tbb::concurrent_bounded_queue<safe_ptr<core::basic_frame>> frame_buffer_;

	std::shared_ptr<flash_renderer> renderer_;
	std::shared_ptr<core::frame_factory> frame_factory_;
	core::video_format_desc format_desc_;
			
	executor executor_;
		
public:
	flash_producer(const std::wstring& filename) 
		: filename_(filename)		
		, tail_(core::basic_frame::empty())		
		, executor_(L"flash_producer")
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));	
		 
		fps_ = 0;
		executor_.start();
	}

	~flash_producer()
	{
		executor_.clear();
		CASPAR_ASSERT(executor_.is_running());
		executor_.invoke([=]
		{
			renderer_ = nullptr;
		});		
	}
	
	virtual void set_frame_factory(const safe_ptr<core::frame_factory>& frame_factory)
	{	
		frame_factory_ = frame_factory;
		format_desc_ = frame_factory->get_video_format_desc();
		frame_buffer_.set_capacity(4);
		graph_ = diagnostics::create_graph([this]{return print();});
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		
		executor_.begin_invoke([=]
		{
			init_renderer();
		});
	}
	
	virtual safe_ptr<core::basic_frame> receive()
	{		
		if(!renderer_)
			return core::basic_frame::empty();
		
		graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));
		if(!frame_buffer_.try_pop(tail_))
			return tail_;

		executor_.begin_invoke([=]
		{
			if(!renderer_)
				return;

			try
			{
				auto frame = core::basic_frame::empty();
				do{frame = renderer_->render_frame(frame_buffer_.size() < frame_buffer_.capacity()-2);}
				while(frame_buffer_.try_push(frame) && frame == core::basic_frame::empty());
				graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
				fps_.fetch_and_store(static_cast<int>(renderer_->fps()*100.0));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				renderer_ = nullptr;
			}
		});			

		return tail_;
	}
	
	virtual void param(const std::wstring& param) 
	{	
		executor_.begin_invoke([=]
		{
			if(!renderer_)
				init_renderer();

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
		
	virtual std::wstring print() const
	{ 
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L", " + 
					boost::lexical_cast<std::wstring>(fps_) + 
					(interlaced(fps_, format_desc_) ? L"i" : L"p") + L"]";		
	}	

	void init_renderer()
	{
		renderer_.reset(new flash_renderer(safe_ptr<diagnostics::graph>(graph_), frame_factory_, filename_));
		while(frame_buffer_.try_push(core::basic_frame::empty())){}		
	}
};

safe_ptr<core::frame_producer> create_flash_producer(const std::vector<std::wstring>& params)
{
	std::wstring filename = env::template_folder() + L"\\" + params[0];
	
	return make_safe<flash_producer>(filename);
}

std::wstring find_flash_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + L".ft")) 
		return template_name + L".ft";
	
	if(boost::filesystem::exists(template_name + L".ct"))
		return template_name + L".ct";

	return L"";
}

}