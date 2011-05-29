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
#pragma warning (disable : 4244)
#endif

#include "flash_producer.h"
#include "FlashAxContainer.h"

#include <core/video_format.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>
#include <common/concurrency/com_context.h>
#include <common/diagnostics/graph.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include <functional>

namespace caspar {
		
class bitmap
{
public:
	bitmap(size_t width, size_t height)
		: bmp_data_(nullptr)
		, hdc_(CreateCompatibleDC(0), DeleteDC)
	{	
		BITMAPINFO info;
		memset(&info, 0, sizeof(BITMAPINFO));
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biHeight = -height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof(BITMAPINFO);
		info.bmiHeader.biWidth = width;

		bmp_.reset(CreateDIBSection(static_cast<HDC>(hdc_.get()), &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), 0, 0), DeleteObject);
		SelectObject(static_cast<HDC>(hdc_.get()), bmp_.get());	

		if(!bmp_data_)
			BOOST_THROW_EXCEPTION(std::bad_alloc());
	}

	operator HDC() {return static_cast<HDC>(hdc_.get());}

	BYTE* data() { return bmp_data_;}
	const BYTE* data() const { return bmp_data_;}

private:
	BYTE* bmp_data_;	
	std::shared_ptr<void> hdc_;
	std::shared_ptr<void> bmp_;
};

class flash_renderer
{	
	const std::wstring filename_;
	core::video_format_desc format_desc_;

	const std::shared_ptr<core::frame_factory> frame_factory_;
	
	CComObject<caspar::flash::FlashAxContainer>* ax_;
	safe_ptr<core::basic_frame> head_;
	bitmap bmp_;
	
	safe_ptr<diagnostics::graph> graph_;
	boost::timer frame_timer_;
	boost::timer tick_timer_;

	high_prec_timer timer_;
	
public:
	flash_renderer(const safe_ptr<diagnostics::graph>& graph, const std::shared_ptr<core::frame_factory>& frame_factory, const std::wstring& filename) 
		: graph_(graph)
		, filename_(filename)
		, format_desc_(frame_factory->get_video_format_desc())
		, frame_factory_(frame_factory)
		, ax_(nullptr)
		, head_(core::basic_frame::empty())
		, bmp_(format_desc_.width, format_desc_.height)
	{
		graph_->add_guide("frame-time", 0.5f);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));	
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->set_color("param", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));			
		
		if(FAILED(CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&ax_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to create FlashAxContainer"));
		
		ax_->set_print([this]{return L"flash_renderer";});

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

		if(abs(ax_->GetFPS() - format_desc_.fps) > 0.01 && abs(ax_->GetFPS()/2.0 - format_desc_.fps) > 0.01)
			CASPAR_LOG(warning) << print() << " Invalid frame-rate: " << ax_->GetFPS() << L". Should be either " << format_desc_.fps << L" or " << format_desc_.fps*2.0 << L".";
	}
	
	safe_ptr<core::basic_frame> render_frame(bool has_underflow)
	{
		if(format_desc_ != frame_factory_->get_video_format_desc())
		{
			format_desc_ = frame_factory_->get_video_format_desc();
			ax_->SetFormat(format_desc_);
		}

		float frame_time = 1.0f/ax_->GetFPS();

		graph_->update_value("tick-time", static_cast<float>(tick_timer_.elapsed()/frame_time)*0.5f);
		tick_timer_.restart();

		if(ax_->IsEmpty())
			return core::basic_frame::empty();		
		
		if(!has_underflow)			
			timer_.tick(frame_time); // This will block the thread.
		else
			graph_->add_tag("underflow");
			
		frame_timer_.restart();

		ax_->Tick();
		if(ax_->InvalidRect())
		{			
			fast_memclr(bmp_.data(),  format_desc_.size);
			ax_->DrawControl(bmp_);
		
			auto frame = frame_factory_->create_frame(this, format_desc_.width, format_desc_.height);
			fast_memcpy(frame->image_data().begin(), bmp_.data(), format_desc_.size);
			frame->commit();
			head_ = frame;
		}		
				
		graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()/frame_time)*0.5f);
		return head_;
	}

	double fps() const
	{
		return ax_->GetFPS();	
	}
	
	std::wstring print()
	{
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"]";		
	}
};

struct flash_producer : public core::frame_producer
{	
	const std::wstring filename_;	
	const safe_ptr<core::frame_factory> frame_factory_;

	tbb::atomic<int> fps_;

	std::shared_ptr<diagnostics::graph> graph_;

	tbb::concurrent_bounded_queue<safe_ptr<core::basic_frame>> frame_buffer_;
				
	com_context<flash_renderer> context_;		
public:
	flash_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename) 
		: filename_(filename)		
		, frame_factory_(frame_factory)
		, context_(L"flash_producer")
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));	
		 
		fps_ = 0;

		graph_ = diagnostics::create_graph([this]{return print();});
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		
		frame_buffer_.set_capacity(2);

		initialize();				
	}

	~flash_producer()
	{
		frame_buffer_.clear();
	}

	// frame_producer
		
	virtual safe_ptr<core::basic_frame> receive()
	{				
		graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));

		auto frame = core::basic_frame::late();
		frame_buffer_.try_pop(frame);		
		return frame;
	}
	
	virtual void param(const std::wstring& param) 
	{	
		context_.begin_invoke([=]
		{
			if(!context_)
				initialize();

			try
			{
				context_->param(param);	
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				context_.reset(nullptr);
				frame_buffer_.push(core::basic_frame::empty());
			}
		});
	}
		
	virtual std::wstring print() const
	{ 
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L", " + 
					boost::lexical_cast<std::wstring>(fps_) + L"]";		
	}	

	// flash_producer

	void initialize()
	{
		context_.reset([&]{return new flash_renderer(safe_ptr<diagnostics::graph>(graph_), frame_factory_, filename_);});
		while(frame_buffer_.try_push(core::basic_frame::empty())){}		
		render(context_.get());
	}

	void render(const flash_renderer* renderer)
	{		
		context_.begin_invoke([=]
		{
			if(context_.get() != renderer) // Since initialize will start a new recursive call make sure the recursive calls are only for a specific instance.
				return;

			try
			{		
				const auto& format_desc = frame_factory_->get_video_format_desc();

				if(abs(context_->fps()/2.0 - format_desc.fps) < 0.01) // flash == 2 * format -> interlace
				{
					auto frame1 = context_->render_frame(frame_buffer_.size() < frame_buffer_.capacity());
					auto frame2 = context_->render_frame(frame_buffer_.size() < frame_buffer_.capacity());
					frame_buffer_.push(core::basic_frame::interlace(frame1, frame2, format_desc.mode));
				}
				else if(abs(context_->fps()- format_desc.fps/2.0) < 0.01) // format == 2 * flash -> duplicate
				{
					auto frame = context_->render_frame(frame_buffer_.size() < frame_buffer_.capacity());
					frame_buffer_.push(frame);
					frame_buffer_.push(frame);
				}
				else //if(abs(renderer_->fps() - format_desc_.fps) < 0.1) // format == flash -> simple
				{
					auto frame = context_->render_frame(frame_buffer_.size() < frame_buffer_.capacity());
					frame_buffer_.push(frame);
				}

				graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
				fps_.fetch_and_store(static_cast<int>(context_->fps()*100.0));

				render(renderer);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				context_.reset(nullptr);
				frame_buffer_.push(core::basic_frame::empty());
			}
		});
	}
};

safe_ptr<core::frame_producer> create_flash_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	std::wstring filename = env::template_folder() + L"\\" + params[0];
	
	return make_safe<flash_producer>(frame_factory, filename);
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