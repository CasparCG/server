/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "html_producer.h"

#include <core/video_format.h>

#include <core/monitor/monitor.h>
#include <core/parameters/parameters.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/concurrency/lock.h>
#include <common/diagnostics/graph.h>
#include <common/utility/timer.h>

#include <berkelium/Berkelium.hpp>
#include <berkelium/Context.hpp>
#include <berkelium/Window.hpp>
#include <berkelium/WindowDelegate.hpp>
#include <berkelium/Rect.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <tbb/atomic.h>
#include <tbb/parallel_for.h>

#include "html.h"

namespace caspar { namespace html {
		
class html_producer 
	: public core::frame_producer
	, public Berkelium::WindowDelegate
{	
	core::monitor::subject				monitor_subject_;
	const std::wstring					url_;	
	safe_ptr<diagnostics::graph>		graph_;
	
	const safe_ptr<core::frame_factory>	frame_factory_;

	safe_ptr<core::basic_frame>			last_frame_;
	mutable boost::mutex				last_frame_mutex_;
	
	std::vector<unsigned char>			frame_;
	mutable boost::mutex				frame_mutex_;

	tbb::atomic<bool>					invalidated_;

	std::unique_ptr<Berkelium::Window>	window_;

	high_prec_timer						timer_;

	executor							executor_;
	
public:
	html_producer(
		const safe_ptr<core::frame_factory>& frame_factory, 
		const std::wstring& url) 
		: url_(url)		
		, frame_factory_(frame_factory)
		, last_frame_(core::basic_frame::empty())
		, frame_(frame_factory->get_video_format_desc().width * frame_factory->get_video_format_desc().height * 4, 0)
		, executor_(L"html_producer")
	{		
		invalidated_ = true;

		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		html::invoke([&]
		{
			{
				std::unique_ptr<Berkelium::Context> context(Berkelium::Context::create());
				window_.reset(Berkelium::Window::create(context.get()));
			}

			window_->resize(
				frame_factory->get_video_format_desc().width,
				frame_factory->get_video_format_desc().height);
			window_->setDelegate(this);
			window_->setTransparent(true);
			
			const auto narrow_url = narrow(url);
			
			if(!window_->navigateTo(
				Berkelium::URLString::point_to(
					narrow_url.data(), 
					narrow_url.length())))
			{
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to navigate."));
			}
		});

		tick();
	}

	~html_producer()
	{
		html::invoke([=]
		{
			window_.reset();
		});
	}
	
	// frame_producer
		
	safe_ptr<core::basic_frame> receive(int) override
	{				
		return last_frame();
	}

	safe_ptr<core::basic_frame> last_frame() const override
	{		
		return lock(last_frame_mutex_, [&]
		{	
			return last_frame_;
		});
	}		
	
	boost::unique_future<std::wstring> call(const std::wstring& param) override
	{	
		static const boost::wregex play_exp(L"PLAY\\s*(\\d+)?", boost::regex::icase);
		static const boost::wregex stop_exp(L"STOP\\s*(\\d+)?", boost::regex::icase);
		static const boost::wregex next_exp(L"NEXT\\s*(\\d+)?", boost::regex::icase);
		static const boost::wregex remove_exp(L"REMOVE\\s*(\\d+)?", boost::regex::icase);
		static const boost::wregex update_exp(L"UPDATE\\s+(\\d+)?\"?(?<VALUE>.*)\"?", boost::regex::icase);
		static const boost::wregex invoke_exp(L"INVOKE\\s+(\\d+)?\"?(?<VALUE>.*)\"?", boost::regex::icase);
		
		boost::packaged_task<std::wstring> task([=]() -> std::wstring
		{
			html::invoke([=]
			{
				auto javascript = param;

				boost::wsmatch what;

				if (boost::regex_match(param, what, play_exp))
				{
					javascript = (boost::wformat(L"play()")).str();
				}
				else if (boost::regex_match(param, what, stop_exp))
				{
					javascript = (boost::wformat(L"stop()")).str();
				}
				else if (boost::regex_match(param, what, next_exp))
				{
					javascript = (boost::wformat(L"next()")).str();
				}
				else if (boost::regex_match(param, what, remove_exp))
				{
					javascript = (boost::wformat(L"remove()")).str();
				}
				else if (boost::regex_match(param, what, update_exp))
				{
					javascript = (boost::wformat(L"update(\"%1%\")") % boost::algorithm::trim_copy_if(what["VALUE"].str(), boost::is_any_of(" \""))).str();
				}
				else if (boost::regex_match(param, what, invoke_exp))
				{
					javascript = (boost::wformat(L"invoke(\"%1%\")") % boost::algorithm::trim_copy_if(what["VALUE"].str(), boost::is_any_of(" \""))).str();
				}
												
				window_->executeJavascript(Berkelium::WideString::point_to(javascript.data(), javascript.length()));
			});

			return L"";
		});

		task();

		return task.get_future();
	}
		
	std::wstring print() const override
	{ 
		return L"html[" + url_ + L"]";		
	}	

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"html-producer");
		return info;
	}
	
	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
	
	// Berkelium::WindowDelegate
	
	void onPaint(
		Berkelium::Window* wini, 
		const unsigned char* bitmap_in, 
		const Berkelium::Rect& bitmap_rect, 
		size_t num_copy_rects, 
		const Berkelium::Rect* copy_rects,	
		int dx, 
		int dy, 
		const Berkelium::Rect& scroll_rect) override
	{		
		lock(frame_mutex_, [&]
		{	
			invalidated_ = true;

			tbb::parallel_for<int>(0, num_copy_rects, 1, [&](int i)
			{									
				tbb::parallel_for<int>(0, copy_rects[i].height(), 1, [&](int y)
				{
					memcpy(
						frame_.data() + ((y + copy_rects[i].top()) * frame_factory_->get_video_format_desc().width + copy_rects[i].left()) * 4,
						bitmap_in + ((y + copy_rects[i].top() - bitmap_rect.top()) * bitmap_rect.width() + copy_rects[i].left() - bitmap_rect.left()) * 4,
						copy_rects[i].width() * 4);
				});
			});		
		});
	}

    void onExternalHost(
        Berkelium::Window *win,
        Berkelium::WideString message,
        Berkelium::URLString origin,
        Berkelium::URLString target) override
	{
	}

	// html_producer
		
	safe_ptr<core::basic_frame> draw(
		safe_ptr<core::write_frame> frame, 
		core::field_mode::type field_mode)
	{
		const auto& pixel_desc = frame->get_pixel_format_desc();

		CASPAR_ASSERT(pixel_desc.pix_fmt == core::pixel_format::bgra);
				
		const auto& height = pixel_desc.planes[0].height;
		const auto& linesize = pixel_desc.planes[0].linesize;

		lock(frame_mutex_, [&]
		{				
			tbb::parallel_for<int>(
				field_mode == core::field_mode::upper ? 0 : 1, 
				height,
				field_mode == core::field_mode::progressive ? 1 : 2,
				[&](int y)
			{
				memcpy(
					frame->image_data().begin() + y * linesize, 
					frame_.data() + y * linesize,
					linesize);
			});
		});
							
		return frame;
	}	

	void tick()
	{
		if(invalidated_.fetch_and_store(false))
		{
			core::pixel_format_desc pixel_desc;
			pixel_desc.pix_fmt = core::pixel_format::bgra;
			pixel_desc.planes.push_back(
				core::pixel_format_desc::plane(
					frame_factory_->get_video_format_desc().width, 
					frame_factory_->get_video_format_desc().height,
					4));

			auto frame = frame_factory_->create_frame(this, pixel_desc);

			const auto& format_desc = frame_factory_->get_video_format_desc();

			if (format_desc.field_mode != core::field_mode::progressive)
			{
				draw(frame, format_desc.field_mode);
				
				executor_.yield();
				timer_.tick(1.0 / (format_desc.fps * format_desc.field_count));
		
				draw(frame, static_cast<core::field_mode::type>(format_desc.field_mode ^ core::field_mode::progressive));
			}
			else
			{				
				draw(frame, format_desc.field_mode);			
			}	
		
			frame->commit();

			lock(last_frame_mutex_, [&]
			{	
				last_frame_ = frame;
			});
			
			executor_.yield();

			timer_.tick(1.0 / (format_desc.fps * format_desc.field_count));
		}

		executor_.begin_invoke([this]{ tick(); });
	}
};

safe_ptr<core::frame_producer> create_producer(
	const safe_ptr<core::frame_factory>& frame_factory,
	const core::parameters& params)
{
	const auto filename = env::template_folder() + L"\\" + params.at_original(0) + L".html";
	
	const auto url = boost::filesystem::exists(filename) 
		? filename 
		: params.at_original(0);
		
	if(!boost::algorithm::contains(url, ".") || boost::algorithm::ends_with(url, "_A") || boost::algorithm::ends_with(url, "_ALPHA"))
		return core::frame_producer::empty();

	return core::create_producer_destroy_proxy(
		core::create_producer_print_proxy(
			make_safe<html_producer>(
				frame_factory,
				url)));
}

}}