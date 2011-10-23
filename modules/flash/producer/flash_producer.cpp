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
#include <common/diagnostics/graph.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include <functional>

#include <agents.h>
#include <agents_extras.h>
#include <concrt_extras.h>

using namespace Concurrency;

namespace caspar { namespace flash {
		
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

struct template_host
{
	std::string  field_mode;
	std::string  filename;
	size_t		 width;
	size_t		 height;
};

template_host get_template_host(const core::video_format_desc& desc)
{
	std::vector<template_host> template_hosts;
	BOOST_FOREACH(auto& xml_mapping, env::properties().get_child("configuration.producers.template-hosts"))
	{
		try
		{
			template_host template_host;
			template_host.field_mode		= xml_mapping.second.get("video-mode", narrow(desc.name));
			template_host.filename			= xml_mapping.second.get("filename", "cg.fth");
			template_host.width				= xml_mapping.second.get("width", desc.width);
			template_host.height			= xml_mapping.second.get("height", desc.height);
			template_hosts.push_back(template_host);
		}
		catch(...){}
	}

	auto template_host_it = boost::find_if(template_hosts, [&](template_host template_host){return template_host.field_mode == narrow(desc.name);});
	if(template_host_it == template_hosts.end())
		template_host_it = boost::find_if(template_hosts, [&](template_host template_host){return template_host.field_mode == "";});

	if(template_host_it != template_hosts.end())
		return *template_host_it;
	
	template_host template_host;
	template_host.filename = "cg.fth";
	template_host.width = desc.width;
	template_host.height = desc.height;
	return template_host;
}

class flash_renderer
{	
	const std::wstring								filename_;

	const safe_ptr<core::frame_factory>				frame_factory_;
	safe_ptr<diagnostics::graph>					graph_;

	high_prec_timer									timer_;
	safe_ptr<core::basic_frame>						head_;
	bitmap											bmp_;

	const size_t									width_;
	const size_t									height_;
			
	boost::timer									frame_timer_;
	boost::timer									tick_timer_;

	CComObject<caspar::flash::FlashAxContainer>*	ax_;
	
public:
	flash_renderer(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, int width, int height) 
		: filename_(filename)
		, frame_factory_(frame_factory)
		, graph_(graph)
		, ax_(nullptr)
		, head_(core::basic_frame::empty())
		, bmp_(width, height)
		, width_(width)
		, height_(height)
	{		
		graph_->add_guide("frame-time", 0.5f);
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("param", diagnostics::color(1.0f, 0.5f, 0.0f));		
		
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
						
		ax_->SetSize(width_, height_);		
	
		CASPAR_LOG(info) << print() << L" Thread started.";
		CASPAR_LOG(info) << print() << L" Successfully initialized with template-host: " << filename << L" width: " << width_ << L" height: " << height_ << L".";
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
			CASPAR_LOG(warning) << print() << L" Flash call failed:" << param;//BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Flash function call failed.") << arg_name_info("param") << arg_value_info(narrow(param)));
		graph_->add_tag("param");
	}
	
	safe_ptr<core::basic_frame> operator()()
	{
		const float frame_time = 1.0f/ax_->GetFPS();

		graph_->update_value("tick-time", static_cast<float>(tick_timer_.elapsed()/frame_time)*0.5f);
		tick_timer_.restart();

		if(ax_->IsEmpty())
			return core::basic_frame::empty();		
		
		Concurrency::scoped_oversubcription_token oversubscribe;
		timer_.tick(frame_time); // This will block the thread.
		//Concurrency::wait(std::max<int>(0, frame_time-3));
			
		frame_timer_.restart();

		ax_->Tick();
		if(ax_->InvalidRect())
		{			
			fast_memclr(bmp_.data(), width_*height_*4);
			ax_->DrawControl(bmp_);
		
			auto frame = frame_factory_->create_frame(this, width_, height_);
			fast_memcpy(frame->image_data().begin(), bmp_.data(), width_*height_*4);
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

struct flash_producer : public Concurrency::agent, public core::frame_producer
{	
	unbounded_buffer<std::wstring>								params_;
	bounded_buffer<safe_ptr<core::basic_frame>>					frames_;

	tbb::atomic<bool>											is_running_;
		
	const safe_ptr<core::frame_factory>							frame_factory_;
	const std::wstring											filename_;	
	tbb::atomic<int>											fps_;
	const int													width_;
	const int													height_;
		
	mutable overwrite_buffer<safe_ptr<core::basic_frame>>		last_frame_;

	safe_ptr<diagnostics::graph>								graph_;
				
public:
	flash_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, size_t width, size_t height) 
		: frames_(frame_factory->get_video_format_desc().fps > 30.0 ? 2 : 1)
		, frame_factory_(frame_factory)
		, filename_(filename)		
		, width_(width > 0 ? width : frame_factory->get_video_format_desc().width)
		, height_(height > 0 ? height : frame_factory->get_video_format_desc().height)
		, graph_(diagnostics::create_graph("flash", false))
	{	
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));	
		
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	

		Concurrency::send(last_frame_, core::basic_frame::empty());
		
		fps_		= 0;
		is_running_ = true;

		graph_->start();
		start();
	}

	~flash_producer()
	{
		is_running_ = false;
		auto frame = core::basic_frame::empty();
		while(try_receive(frames_, frame)){}
		agent::wait(this);
	}
	
	virtual void run()
	{		
		try
		{
			struct co_init
			{
				co_init()  {CoInitialize(NULL);}
				~co_init() {CoUninitialize();}
			} init;

			flash_renderer renderer(graph_, frame_factory_, filename_, width_, height_);

			is_running_ = true;
			while(is_running_)
			{
				std::wstring param;
				while(is_running_ && Concurrency::try_receive(params_, param))
					renderer.param(param);
			
				const auto& format_desc = frame_factory_->get_video_format_desc();

				if(abs(renderer.fps()/2.0 - format_desc.fps) < 2.0) // flash == 2 * format -> interlace
				{
					auto frame1 = renderer();
					auto frame2 = renderer();
					send(last_frame_, frame2);
					send(frames_, core::basic_frame::interlace(frame1, frame2, format_desc.field_mode));
				}
				else if(abs(renderer.fps()- format_desc.fps/2.0) < 2.0) // format == 2 * flash -> duplicate
				{
					auto frame = renderer();
					send(last_frame_, frame);
					send(frames_, frame);
					send(frames_, frame);
				}
				else //if(abs(renderer_->fps() - format_desc_.fps) < 0.1) // format == flash -> simple
				{
					auto frame = renderer();
					send(last_frame_, frame);
					send(frames_, frame);
				}

				fps_ = static_cast<int>(renderer.fps()*100.0);
				graph_->update_text(narrow(print()));
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		is_running_ = false;
		done();
	}
	
	// frame_producer
		
	virtual safe_ptr<core::basic_frame> receive(int)
	{						
		auto frame = core::basic_frame::late();

		try
		{
			frame = Concurrency::receive(frames_, 5);
		}
		catch(operation_timed_out&)
		{			
			graph_->add_tag("underflow");
		}

		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return last_frame_.value();
	}		
	
	virtual void param(const std::wstring& param) 
	{	
		if(!is_running_.fetch_and_store(true))
		{
			agent::wait(this);
			start();
		}
		asend(params_, param);
	}
		
	virtual std::wstring print() const
	{ 
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"|" + boost::lexical_cast<std::wstring>(fps_) + L"]";		
	}	
};

safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	auto template_host = get_template_host(frame_factory->get_video_format_desc());
	
	return make_safe<flash_producer>(frame_factory, env::template_folder() + L"\\" + widen(template_host.filename), template_host.width, template_host.height);
}

std::wstring find_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + L".ft")) 
		return template_name + L".ft";
	
	if(boost::filesystem::exists(template_name + L".ct"))
		return template_name + L".ct";

	return L"";
}

}}