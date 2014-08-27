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

#include <common/utility/assert.h>
#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/concurrency/lock.h>
#include <common/diagnostics/graph.h>
#include <common/utility/timer.h>
#include <common/memory/memcpy.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <tbb/atomic.h>
#include <tbb/parallel_for.h>

#include <cef_task.h>
#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>

#include "html.h"

#pragma comment (lib, "libcef.lib")
#pragma comment (lib, "libcef_dll_wrapper.lib")

namespace caspar {
	namespace html {
		
		class html_client
			: public CefClient
			, public CefRenderHandler
			, public CefLifeSpanHandler
		{

			safe_ptr<core::frame_factory>	frame_factory_;
			tbb::atomic<bool>				invalidated_;
			std::vector<unsigned char>		frame_;
			mutable boost::mutex			frame_mutex_;

			safe_ptr<core::basic_frame>		last_frame_;
			mutable boost::mutex			last_frame_mutex_;

			CefRefPtr<CefBrowser>			browser_;

			executor						executor_;

		public:

			html_client(safe_ptr<core::frame_factory> frame_factory)
				: frame_factory_(frame_factory)
				, frame_(frame_factory->get_video_format_desc().width * frame_factory->get_video_format_desc().height * 4, 0)
				, last_frame_(core::basic_frame::empty())
				, executor_(L"html_producer")
			{
				invalidated_ = true;
				executor_.begin_invoke([&]{ update(); });
			}

			safe_ptr<core::basic_frame> receive(int)
			{
				auto frame = last_frame();
				executor_.begin_invoke([&]{ update(); });
				return frame;
			}

			safe_ptr<core::basic_frame> last_frame() const
			{
				return lock(last_frame_mutex_, [&]
				{
					return last_frame_;
				});
			}

			void execute_javascript(const std::wstring& javascript)
			{
				html::begin_invoke([=]
				{
					if (browser_ != nullptr)
						browser_->GetMainFrame()->ExecuteJavaScript(narrow(javascript).c_str(), browser_->GetMainFrame()->GetURL(), 0);
				});
			}

			void close()
			{
				html::invoke([=]
				{
					if (browser_ != nullptr)
					{
						browser_->GetHost()->CloseBrowser(true);
					}
				});
			}

		private:

			bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				rect = CefRect(0, 0, frame_factory_->get_video_format_desc().width, frame_factory_->get_video_format_desc().height);
				return true;
			}

			void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				lock(frame_mutex_, [&]
				{
					invalidated_ = true;
					fast_memcpy(frame_.data(), buffer, width * height * 4);
				});
			}

			void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				browser_ = browser;
			}

			void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				browser_ = nullptr;
			}

			bool DoClose(CefRefPtr<CefBrowser> browser) override
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				return false;
			}

			CefRefPtr<CefRenderHandler> GetRenderHandler() override
			{
				return this;
			}

			CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
			{
				return this;
			}

			void invoke_on_enter_frame()
			{
				//html::invoke([this]
				//{
				//	static const std::wstring javascript = L"onEnterFrame()";
				//	execute_javascript(javascript);
				//});
			}

			safe_ptr<core::basic_frame> draw(safe_ptr<core::write_frame> frame, core::field_mode::type field_mode)
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
						fast_memcpy(
							frame->image_data().begin() + y * linesize,
							frame_.data() + y * linesize,
							linesize);
					});
				});

				return frame;
			}

			void update()
			{
				invoke_on_enter_frame();

				if (invalidated_.fetch_and_store(false))
				{
					high_prec_timer timer;
					timer.tick(0.0);

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
						timer.tick(1.0 / (format_desc.fps * format_desc.field_count));

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
				}
			}

			IMPLEMENT_REFCOUNTING(html_client);
		};

		class html_producer
			: public core::frame_producer
		{
			core::monitor::subject				monitor_subject_;
			const std::wstring					url_;
			safe_ptr<diagnostics::graph>		graph_;

			CefRefPtr<html_client>				client_;

		public:
			html_producer(
				const safe_ptr<core::frame_factory>& frame_factory,
				const std::wstring& url)
				: url_(url)
			{
				graph_->set_text(print());
				diagnostics::register_graph(graph_);

				html::invoke([&]
				{
					client_ = new html_client(frame_factory);

					CefWindowInfo window_info;

					window_info.SetTransparentPainting(TRUE);
					window_info.SetAsOffScreen(nullptr);
					//window_info.SetAsWindowless(nullptr, true);
					
					CefBrowserSettings browser_settings;	
					CefBrowserHost::CreateBrowser(window_info, client_.get(), url, browser_settings, nullptr);
				});
			}

			~html_producer()
			{
				client_->close();
			}

			// frame_producer

			safe_ptr<core::basic_frame> receive(int) override
			{
				return client_->receive(0);
			}

			safe_ptr<core::basic_frame> last_frame() const override
			{
				return client_->last_frame();
			}

			boost::unique_future<std::wstring> call(const std::wstring& param) override
			{
				static const boost::wregex play_exp(L"PLAY\\s*(\\d+)?", boost::regex::icase);
				static const boost::wregex stop_exp(L"STOP\\s*(\\d+)?", boost::regex::icase);
				static const boost::wregex next_exp(L"NEXT\\s*(\\d+)?", boost::regex::icase);
				static const boost::wregex remove_exp(L"REMOVE\\s*(\\d+)?", boost::regex::icase);
				static const boost::wregex update_exp(L"UPDATE\\s+(\\d+)?\"?(?<VALUE>.*)\"?", boost::regex::icase);
				static const boost::wregex invoke_exp(L"INVOKE\\s+(\\d+)?\"?(?<VALUE>.*)\"?", boost::regex::icase);

				auto command = [=]
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

					client_->execute_javascript(javascript);
				};

				boost::packaged_task<std::wstring> task([=]() -> std::wstring
				{
					html::invoke(command);
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
		};

		safe_ptr<core::frame_producer> create_producer(
			const safe_ptr<core::frame_factory>& frame_factory,
			const core::parameters& params)
		{
			const auto filename = env::template_folder() + L"\\" + params.at_original(0) + L".html";
	
			if (!boost::filesystem::exists(filename) && params.at(0) != L"[HTML]")
				return core::frame_producer::empty();

			const auto url = boost::filesystem::exists(filename) 
				? filename
				: params.at_original(1);
		
			if(!boost::algorithm::contains(url, ".") || boost::algorithm::ends_with(url, "_A") || boost::algorithm::ends_with(url, "_ALPHA"))
				return core::frame_producer::empty();

			return core::create_producer_destroy_proxy(
				core::create_producer_print_proxy(
					make_safe<html_producer>(
						frame_factory,
						url)));
		}

	}
}