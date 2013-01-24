/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../stdafx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4146)
#pragma warning (disable : 4244)
#endif

#include "flash_producer.h"
#include "FlashAxContainer.h"

#include "../util/swf.h"

#include <core/video_format.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/concurrency/lock.h>
#include <common/diagnostics/graph.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>
#include <boost/algorithm/string.hpp>

#include <functional>

#include <tbb/spin_mutex.h>

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
	std::wstring  video_mode;
	std::wstring  filename;
	size_t		  width;
	size_t		  height;
};

template_host get_template_host(const core::video_format_desc& desc)
{
	try
	{
		std::vector<template_host> template_hosts;
		BOOST_FOREACH(auto& xml_mapping, env::properties().get_child(L"configuration.template-hosts"))
		{
			try
			{
				template_host template_host;
				template_host.video_mode		= xml_mapping.second.get(L"video-mode", L"");
				template_host.filename			= xml_mapping.second.get(L"filename",	L"cg.fth");
				template_host.width				= xml_mapping.second.get(L"width",		desc.width);
				template_host.height			= xml_mapping.second.get(L"height",		desc.height);
				template_hosts.push_back(template_host);
			}
			catch(...){}
		}

		auto template_host_it = boost::find_if(template_hosts, [&](template_host template_host){return template_host.video_mode == desc.name;});
		if(template_host_it == template_hosts.end())
			template_host_it = boost::find_if(template_hosts, [&](template_host template_host){return template_host.video_mode == L"";});

		if(template_host_it != template_hosts.end())
			return *template_host_it;
	}
	catch(...){}
		
	template_host template_host;
	template_host.filename = L"cg.fth";

	for(auto it = boost::filesystem2::wdirectory_iterator(env::template_folder()); it != boost::filesystem2::wdirectory_iterator(); ++it)
	{
		if(boost::iequals(it->path().extension(), L"." + desc.name))
		{
			template_host.filename = it->filename();
			break;
		}
	}

	template_host.width =  desc.square_width;
	template_host.height = desc.square_height;
	return template_host;
}

class flash_renderer
{	
	struct com_init
	{
		HRESULT result_;

		com_init()
			: result_(CoInitialize(nullptr))
		{
			if(FAILED(result_))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to initialize com-context for flash-player"));
		}

		~com_init()
		{
			if(SUCCEEDED(result_))
				::CoUninitialize();
		}
	} com_init_;
	
	const safe_ptr<diagnostics::graph>				graph_;
	const size_t									width_;
	const size_t									height_;
	const std::wstring								filename_;
	const std::shared_ptr<core::frame_factory>		frame_factory_;
	
	CComObject<caspar::flash::FlashAxContainer>*	ax_;
	safe_ptr<core::basic_frame>						head_;
	bitmap											bmp_;
	
	boost::timer									frame_timer_;
	boost::timer									tick_timer_;

	high_prec_timer									timer_;
	
public:
	flash_renderer(const safe_ptr<diagnostics::graph>& graph, const std::shared_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, int width, int height) 
		: graph_(graph)
		, width_(width)
		, height_(height)
		, filename_(filename)
		, frame_factory_(frame_factory)
		, ax_(nullptr)
		, head_(core::basic_frame::empty())
		, bmp_(width, height)
	{		
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("param", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("sync", diagnostics::color(0.8f, 0.3f, 0.2f));			
		
		if(FAILED(CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&ax_)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to create FlashAxContainer"));
		
		ax_->set_print([this]{return print();});

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
		render_frame(false);
	
		CASPAR_LOG(info) << print() << L" Initialized.";
	}

	~flash_renderer()
	{		
		if(ax_)
		{
			ax_->DestroyAxControl();
			ax_->Release();
		}
		graph_->set_value("tick-time", 0.0f);
		graph_->set_value("frame-time", 0.0f);
		CASPAR_LOG(info) << print() << L" Uninitialized.";
	}
	
	std::wstring call(const std::wstring& param)
	{		
		std::wstring result;

		CASPAR_LOG(trace) << print() << " Call: " << param;

		if(!ax_->FlashCall(param, result))
			CASPAR_LOG(warning) << print() << L" Flash call failed:" << param;//BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Flash function call failed.") << arg_name_info("param") << arg_value_info(narrow(param)));
		graph_->set_tag("param");

		return result;
	}
	
	safe_ptr<core::basic_frame> render_frame(double sync)
	{
		float frame_time = 1.0f/ax_->GetFPS();

		graph_->set_value("tick-time", static_cast<float>(tick_timer_.elapsed()/frame_time)*0.5f);
		tick_timer_.restart();

		if(ax_->IsEmpty())
			return core::basic_frame::empty();		
		
		if(sync > 0.00001)			
			timer_.tick(frame_time*sync); // This will block the thread.
		else
			graph_->set_tag("sync");

		graph_->set_value("sync", sync);
			
		frame_timer_.restart();

		ax_->Tick();
		if(ax_->InvalidRect())
		{
			fast_memclr(bmp_.data(), width_*height_*4);
			ax_->DrawControl(bmp_);
		
			core::pixel_format_desc desc;
			desc.pix_fmt = core::pixel_format::bgra;
			desc.planes.push_back(core::pixel_format_desc::plane(width_, height_, 4));
			auto frame = frame_factory_->create_frame(this, desc);

			if(frame->image_data().size() == static_cast<int>(width_*height_*4))
			{
				fast_memcpy(frame->image_data().begin(), bmp_.data(), width_*height_*4);
				frame->commit();
				head_ = frame;
			}
		}		
					
		MSG msg;
		while(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) // DO NOT REMOVE THE MESSAGE DISPATCH LOOP. Without this some stuff doesn't work!  
		{
			if(msg.message == WM_TIMER && msg.wParam == 3 && msg.lParam == 0) // We tick this inside FlashAxContainer
				continue;
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);			
		}
										
		graph_->set_value("frame-time", static_cast<float>(frame_timer_.elapsed()/frame_time)*0.5f);
		return head_;
	}

	bool is_empty() const
	{
		return ax_->IsEmpty();
	}

	double fps() const
	{
		return ax_->GetFPS();	
	}
	
	std::wstring print()
	{
		return L"flash-player[" + boost::filesystem::wpath(filename_).filename() 
				  + L"|" + boost::lexical_cast<std::wstring>(width_)
				  + L"x" + boost::lexical_cast<std::wstring>(height_)
				  + L"]";		
	}
};

struct flash_producer : public core::frame_producer
{	
	const std::wstring											filename_;	
	const safe_ptr<core::frame_factory>							frame_factory_;
	const int													width_;
	const int													height_;
	const int													buffer_size_;

	tbb::atomic<int>											fps_;

	safe_ptr<diagnostics::graph>								graph_;

	std::queue<safe_ptr<core::basic_frame>>						frame_buffer_;
	tbb::concurrent_bounded_queue<safe_ptr<core::basic_frame>>	output_buffer_;
	
	mutable tbb::spin_mutex										last_frame_mutex_;
	safe_ptr<core::basic_frame>									last_frame_;
		
	std::unique_ptr<flash_renderer>								renderer_;

	executor													executor_;	
public:
	flash_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, size_t width, size_t height) 
		: filename_(filename)		
		, frame_factory_(frame_factory)
		, last_frame_(core::basic_frame::empty())
		, width_(width > 0 ? width : frame_factory->get_video_format_desc().width)
		, height_(height > 0 ? height : frame_factory->get_video_format_desc().height)
		, buffer_size_(env::properties().get(L"configuration.flash.buffer-depth", frame_factory_->get_video_format_desc().fps > 30.0 ? 4 : 2))
		, executor_(L"flash_producer")
	{	
		fps_ = 0;
	 
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.9f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		renderer_.reset(new flash_renderer(graph_, frame_factory_, filename_, width_, height_));

		while(output_buffer_.size() < buffer_size_)
			output_buffer_.push(core::basic_frame::empty());
	}

	~flash_producer()
	{
		executor_.invoke([this]
		{
			renderer_.reset();
		}, high_priority);
	}

	// frame_producer
		
	virtual safe_ptr<core::basic_frame> receive(int) override
	{					
		auto frame = core::basic_frame::late();
		
		if(output_buffer_.try_pop(frame))	
			next();
		else
			graph_->set_tag("late-frame");

		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return lock(last_frame_mutex_, [this]
		{
			return last_frame_;
		});
	}		
	
	virtual boost::unique_future<std::wstring> call(const std::wstring& param) override
	{	
		return executor_.begin_invoke([this, param]() -> std::wstring
		{			
			try
			{
				if(!renderer_)
				{
					renderer_.reset(new flash_renderer(graph_, frame_factory_, filename_, width_, height_));

					while(output_buffer_.size() < buffer_size_)
						output_buffer_.push(core::basic_frame::empty());
				}

				return renderer_->call(param);	

				//const auto& format_desc = frame_factory_->get_video_format_desc();
				//if(abs(context_->fps() - format_desc.fps) > 0.01 && abs(context_->fps()/2.0 - format_desc.fps) > 0.01)
				//	CASPAR_LOG(warning) << print() << " Invalid frame-rate: " << context_->fps() << L". Should be either " << format_desc.fps << L" or " << format_desc.fps*2.0 << L".";
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				renderer_.reset(nullptr);
			}

			return L"";
		});
	}
		
	virtual std::wstring print() const override
	{ 
		return L"flash[" + boost::filesystem::wpath(filename_).filename() + L"|" + boost::lexical_cast<std::wstring>(fps_) + L"]";		
	}	

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"flash-producer");
		return info;
	}

	// flash_producer
	
	void next()
	{	
		executor_.begin_invoke([this]
		{
			if(!renderer_)
				frame_buffer_.push(core::basic_frame::empty());

			if(frame_buffer_.empty())
			{
				auto format_desc = frame_factory_->get_video_format_desc();
					
				if(abs(renderer_->fps()/2.0 - format_desc.fps) < 2.0) // flash == 2 * format -> interlace
				{
					auto frame1 = render_frame();
					auto frame2 = render_frame();
					frame_buffer_.push(core::basic_frame::interlace(frame1, frame2, format_desc.field_mode));
				}
				else if(abs(renderer_->fps() - format_desc.fps/2.0) < 2.0) // format == 2 * flash -> duplicate
				{
					auto frame = render_frame();
					frame_buffer_.push(frame);
					frame_buffer_.push(frame);
				}
				else //if(abs(renderer_->fps() - format_desc_.fps) < 0.1) // format == flash -> simple
				{
					auto frame = render_frame();
					frame_buffer_.push(frame);
				}
						
				fps_.fetch_and_store(static_cast<int>(renderer_->fps()*100.0));				
				graph_->set_text(print());
			
				if(renderer_->is_empty())			
					renderer_.reset();
			}

			output_buffer_.push(std::move(frame_buffer_.front()));
			frame_buffer_.pop();
		});
	}

	safe_ptr<core::basic_frame> render_frame()
	{	
		double ratio = std::min(1.0, static_cast<double>(output_buffer_.size())/static_cast<double>(std::max(1, buffer_size_ - 1)));
		double sync  = 2*ratio - ratio*ratio;

		auto frame = renderer_->render_frame(sync);
		lock(last_frame_mutex_, [&]
		{
			last_frame_ = frame;
		});
		return frame;
	}
};

safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	auto template_host = get_template_host(frame_factory->get_video_format_desc());
	
	auto filename = env::template_folder() + L"\\" + template_host.filename;
	
	if(!boost::filesystem::exists(filename))
		BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));	

	return create_producer_destroy_proxy(
		   create_producer_print_proxy(
			make_safe<flash_producer>(frame_factory, filename, template_host.width, template_host.height)));
}

safe_ptr<core::frame_producer> create_swf_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params) 
{
	auto filename = env::media_folder() + L"\\" + params.at(0) + L".swf";
	
	if(!boost::filesystem::exists(filename))
		return core::frame_producer::empty();

	swf_t::header_t header(filename);

	return create_producer_destroy_proxy(
		   create_producer_print_proxy(
			make_safe<flash_producer>(frame_factory, filename, header.frame_width, header.frame_height)));
}

std::wstring find_template(const std::wstring& template_name)
{
	if(boost::filesystem::exists(template_name + L".ft")) 
		return template_name + L".ft";
	
	if(boost::filesystem::exists(template_name + L".ct"))
		return template_name + L".ct";
	
	if(boost::filesystem::exists(template_name + L".swf"))
		return template_name + L".swf";

	return L"";
}

}}