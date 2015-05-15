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
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>
#include <core/frame/geometry.h>

#include <common/assert.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/lock.h>
#include <common/future.h>
#include <common/diagnostics/graph.h>
#include <common/prec_timer.h>
#include <common/linq.h>
#include <common/os/filesystem.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <cef_task.h>
#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>

#include <asmlib.h>

#include <queue>

#include "../html.h"

#pragma comment (lib, "libcef.lib")
#pragma comment (lib, "libcef_dll_wrapper.lib")

namespace caspar {
	namespace html {
		
		class html_client
			: public CefClient
			, public CefRenderHandler
			, public CefLifeSpanHandler
			, public CefLoadHandler
		{
			std::wstring							url_;
			spl::shared_ptr<diagnostics::graph>		graph_;
			boost::timer							tick_timer_;
			boost::timer							frame_timer_;
			boost::timer							paint_timer_;

			spl::shared_ptr<core::frame_factory>	frame_factory_;
			core::video_format_desc					format_desc_;
			tbb::concurrent_queue<std::wstring>		javascript_before_load_;
			tbb::atomic<bool>						loaded_;
			tbb::atomic<bool>						removed_;
			tbb::atomic<bool>						animation_frame_requested_;
			std::queue<core::draw_frame>			frames_;
			mutable boost::mutex					frames_mutex_;

			core::draw_frame						last_frame_;
			core::draw_frame						last_progressive_frame_;
			mutable boost::mutex					last_frame_mutex_;

			CefRefPtr<CefBrowser>					browser_;

			executor								executor_;

		public:

			html_client(
					spl::shared_ptr<core::frame_factory> frame_factory,
					const core::video_format_desc& format_desc,
					const std::wstring& url)
				: url_(url)
				, frame_factory_(std::move(frame_factory))
				, format_desc_(format_desc)
				, last_frame_(core::draw_frame::empty())
				, last_progressive_frame_(core::draw_frame::empty())
				, executor_(L"html_producer")
			{
				graph_->set_color("browser-tick-time", diagnostics::color(0.1f, 1.0f, 0.1f));
				graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
				graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.9f));
				graph_->set_text(print());
				diagnostics::register_graph(graph_);

				loaded_ = false;
				removed_ = false;
				animation_frame_requested_ = false;
				executor_.begin_invoke([&]{ update(); });
			}

			core::draw_frame receive()
			{
				auto frame = last_frame();
				executor_.begin_invoke([&]{ update(); });
				return frame;
			}

			core::draw_frame last_frame() const
			{
				return lock(last_frame_mutex_, [&]
				{
					return last_frame_;
				});
			}

			void execute_javascript(const std::wstring& javascript)
			{
				if (!loaded_)
				{
					javascript_before_load_.push(javascript);
				}
				else
				{
					execute_queued_javascript();
					do_execute_javascript(javascript);
				}
			}

			void close()
			{
				if (!animation_frame_requested_)
					CASPAR_LOG(warning) << print()
							<< " window.requestAnimationFrame() never called. "
							<< "Animations might have been laggy";

				html::invoke([=]
				{
					if (browser_ != nullptr)
					{
						browser_->GetHost()->CloseBrowser(true);
					}
				});
			}

			void remove()
			{
				close();
				removed_ = true;
			}

			bool is_removed() const
			{
				return removed_;
			}

		private:

			bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
			{
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				rect = CefRect(0, 0, format_desc_.square_width, format_desc_.square_height);
				return true;
			}

			void OnPaint(
					CefRefPtr<CefBrowser> browser,
					PaintElementType type,
					const RectList &dirtyRects,
					const void *buffer,
					int width,
					int height)
			{
				graph_->set_value("browser-tick-time", paint_timer_.elapsed()
						* format_desc_.fps
						* format_desc_.field_count
						* 0.5);
				paint_timer_.restart();
				CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

				boost::timer copy_timer;
				core::pixel_format_desc pixel_desc;
					pixel_desc.format = core::pixel_format::bgra;
					pixel_desc.planes.push_back(
						core::pixel_format_desc::plane(width, height, 4));
				auto frame = frame_factory_->create_frame(this, pixel_desc);
				A_memcpy(frame.image_data().begin(), buffer, width * height * 4);

				lock(frames_mutex_, [&]
				{
					frames_.push(core::draw_frame(std::move(frame)));

					size_t max_in_queue = format_desc_.field_count;

					while (frames_.size() > max_in_queue)
					{
						frames_.pop();
						graph_->set_tag("dropped-frame");
					}
				});
				graph_->set_value("copy-time", copy_timer.elapsed()
						* format_desc_.fps
						* format_desc_.field_count
						* 0.5);
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

			CefRefPtr<CefLoadHandler> GetLoadHandler() override {
				return this;
			}

			void OnLoadEnd(
					CefRefPtr<CefBrowser> browser,
					CefRefPtr<CefFrame> frame,
					int httpStatusCode) override
			{
				loaded_ = true;
				execute_queued_javascript();
			}

			bool OnProcessMessageReceived(
					CefRefPtr<CefBrowser> browser,
					CefProcessId source_process,
					CefRefPtr<CefProcessMessage> message) override
			{
				auto name = message->GetName().ToString();

				if (name == ANIMATION_FRAME_REQUESTED_MESSAGE_NAME)
				{
					CASPAR_LOG(trace)
							<< print() << L" Requested animation frame";
					animation_frame_requested_ = true;

					return true;
				}
				else if (name == REMOVE_MESSAGE_NAME)
				{
					remove();

					return true;
				}
				else if (name == LOG_MESSAGE_NAME)
				{
					auto args = message->GetArgumentList();
					auto severity =
						static_cast<boost::log::trivial::severity_level>(args->GetInt(0));
					auto msg = args->GetString(1).ToWString();

					BOOST_LOG_STREAM_WITH_PARAMS(
							log::logger::get(),
							(boost::log::keywords::severity = severity))
						<< print() << L" [renderer_process] " << msg;
				}

				return false;
			}

			void invoke_requested_animation_frames()
			{
				if (browser_)
					browser_->SendProcessMessage(
							CefProcessId::PID_RENDERER,
							CefProcessMessage::Create(TICK_MESSAGE_NAME));
				graph_->set_value("tick-time", tick_timer_.elapsed()
						* format_desc_.fps
						* format_desc_.field_count
						* 0.5);
				tick_timer_.restart();
			}

			bool try_pop(core::draw_frame& result)
			{
				return lock(frames_mutex_, [&]() -> bool
				{
					if (!frames_.empty())
					{
						result = std::move(frames_.front());
						frames_.pop();

						return true;
					}

					return false;
				});
			}

			core::draw_frame pop()
			{
				core::draw_frame frame;

				if (!try_pop(frame))
				{
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(u8(print()) + " No frame in buffer"));
				}

				return frame;
			}

			void update()
			{
				invoke_requested_animation_frames();

				prec_timer timer;
				timer.tick(0.0);

				auto num_frames = lock(frames_mutex_, [&]
				{
					return frames_.size();
				});

				if (num_frames >= format_desc_.field_count)
				{
					if (format_desc_.field_mode != core::field_mode::progressive)
					{
						auto frame1 = pop();

						executor_.yield(caspar::task_priority::lowest_priority);
						timer.tick(1.0 / (format_desc_.fps * format_desc_.field_count));
						invoke_requested_animation_frames();

						auto frame2 = pop();

						lock(last_frame_mutex_, [&]
						{
							last_progressive_frame_ = frame2;
							last_frame_ = core::draw_frame::interlace(frame1, frame2, format_desc_.field_mode);
						});
					}
					else
					{
						auto frame = pop();

						lock(last_frame_mutex_, [&]
						{
							last_frame_ = frame;
						});
					}
				}
				else if (num_frames == 1) // Interlaced but only one frame
				{                         // available. Probably the last frame
				                          // of some animation sequence.
					auto frame = pop();

					lock(last_frame_mutex_, [&]
					{
						last_progressive_frame_ = frame;
						last_frame_ = frame;
					});

					timer.tick(1.0 / (format_desc_.fps * format_desc_.field_count));
					invoke_requested_animation_frames();
				}
				else
				{
					graph_->set_tag("late-frame");

					if (format_desc_.field_mode != core::field_mode::progressive)
					{
						lock(last_frame_mutex_, [&]
						{
							last_frame_ = last_progressive_frame_;
						});

						timer.tick(1.0 / (format_desc_.fps * format_desc_.field_count));
						invoke_requested_animation_frames();
					}
				}
			}

			void do_execute_javascript(const std::wstring& javascript)
			{
				html::begin_invoke([=]
				{
					if (browser_ != nullptr)
						browser_->GetMainFrame()->ExecuteJavaScript(u8(javascript).c_str(), browser_->GetMainFrame()->GetURL(), 0);
				});
			}

			void execute_queued_javascript()
			{
				std::wstring javascript;

				while (javascript_before_load_.try_pop(javascript))
					do_execute_javascript(javascript);
			}

			std::wstring print() const
			{
				return L"html[" + url_ + L"]";
			}

			IMPLEMENT_REFCOUNTING(html_client);
		};

		class html_producer
			: public core::frame_producer_base
		{
			core::monitor::subject				monitor_subject_;
			const std::wstring					url_;
			core::constraints					constraints_;

			CefRefPtr<html_client>				client_;

		public:
			html_producer(
				const spl::shared_ptr<core::frame_factory>& frame_factory,
				const core::video_format_desc& format_desc,
				const std::wstring& url)
				: url_(url)
			{
				constraints_.width.set(format_desc.square_width);
				constraints_.height.set(format_desc.square_height);

				html::invoke([&]
				{
					client_ = new html_client(frame_factory, format_desc, url_);

					CefWindowInfo window_info;

					window_info.SetTransparentPainting(true);
					window_info.SetAsOffScreen(nullptr);
					//window_info.SetAsWindowless(nullptr, true);
					
					CefBrowserSettings browser_settings;
					CefBrowserHost::CreateBrowser(window_info, client_.get(), url, browser_settings, nullptr);
				});
			}

			~html_producer()
			{
				if (client_)
					client_->close();
			}

			// frame_producer

			std::wstring name() const override
			{
				return L"html";
			}

			core::draw_frame receive_impl() override
			{
				if (client_)
				{
					if (client_->is_removed())
					{
						client_ = nullptr;
						return core::draw_frame::empty();
					}

					return client_->receive();
				}

				return core::draw_frame::empty();
			}

			std::future<std::wstring> call(const std::vector<std::wstring>& params) override
			{
				if (!client_)
					return make_ready_future(std::wstring(L""));

				auto javascript = params.at(0);

				client_->execute_javascript(javascript);

				return make_ready_future(std::wstring(L""));
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

			core::constraints& pixel_constraints() override
			{
				return constraints_;
			}

			core::monitor::subject& monitor_output()
			{
				return monitor_subject_;
			}
		};

		spl::shared_ptr<core::frame_producer> create_producer(
				const spl::shared_ptr<core::frame_factory>& frame_factory,
				const core::video_format_desc& format_desc,
				const std::vector<std::wstring>& params)
		{
			const auto filename = env::template_folder() + params.at(0) + L".html";
			const auto found_filename = find_case_insensitive(filename);

			if (!found_filename && !boost::iequals(params.at(0), L"[HTML]"))
				return core::frame_producer::empty();

			const auto url = found_filename 
				? L"file://" + *found_filename
				: params.at(1);
		
			if (!boost::algorithm::contains(url, ".") || boost::algorithm::ends_with(url, "_A") || boost::algorithm::ends_with(url, "_ALPHA"))
				return core::frame_producer::empty();

			return core::create_destroy_proxy(spl::make_shared<html_producer>(
					frame_factory,
					format_desc,
					url));
		}
	}
}
