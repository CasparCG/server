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

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/os/filesystem.h>
#include <common/timer.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <mutex>

#pragma warning(push)
#pragma warning(disable : 4458)
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/cef_task.h>
#pragma warning(pop)

#include <queue>

#include "../html.h"

#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

namespace caspar { namespace html {

class html_client
    : public CefClient
    , public CefRenderHandler
    , public CefLifeSpanHandler
    , public CefLoadHandler
{
    std::wstring                        url_;
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;
    caspar::timer                       frame_timer_;
    caspar::timer                       paint_timer_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    core::video_format_desc              format_desc_;
    tbb::concurrent_queue<std::wstring>  javascript_before_load_;
    std::atomic<bool>                    loaded_;
    std::atomic<bool>                    removed_;
    std::queue<core::draw_frame>         frames_;
    mutable std::mutex                   frames_mutex_;

    core::draw_frame   last_frame_;
    mutable std::mutex last_frame_mutex_;

    CefRefPtr<CefBrowser> browser_;

    executor executor_;

  public:
    html_client(spl::shared_ptr<core::frame_factory> frame_factory,
                spl::shared_ptr<diagnostics::graph>  graph,
                const core::video_format_desc&       format_desc,
                const std::wstring&                  url)
        : url_(url)
        , graph_(graph)
        , frame_factory_(std::move(frame_factory))
        , format_desc_(format_desc)
        , executor_(L"html_producer")
    {
        graph_->set_color("browser-tick-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.1f, 0.1f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        loaded_  = false;
        removed_ = false;
        executor_.begin_invoke([&] { update(); });
    }

    core::draw_frame receive()
    {
        auto frame = last_frame();
        executor_.begin_invoke([&] { update(); });
        return frame;
    }

    core::draw_frame last_frame() const
    {
        std::lock_guard<std::mutex> lock(last_frame_mutex_);
        return last_frame_;
    }

    void execute_javascript(const std::wstring& javascript)
    {
        if (!loaded_) {
            javascript_before_load_.push(javascript);
        } else {
            execute_queued_javascript();
            do_execute_javascript(javascript);
        }
    }

    bool OnBeforePopup(CefRefPtr<CefBrowser>   browser,
                       CefRefPtr<CefFrame>     frame,
                       const CefString&        target_url,
                       const CefString&        target_frame_name,
                       WindowOpenDisposition   target_disposition,
                       bool                    user_gesture,
                       const CefPopupFeatures& popupFeatures,
                       CefWindowInfo&          windowInfo,
                       CefRefPtr<CefClient>&   client,
                       CefBrowserSettings&     settings,
                       bool*                   no_javascript_access) override
    {
        // This blocks popup windows from opening, as they dont make sense and hit an exception in get_browser_host upon
        // closing
        return true;
    }

    CefRefPtr<CefBrowserHost> get_browser_host() const
    {
        if (browser_)
            return browser_->GetHost();
        return nullptr;
    }

    void close()
    {
        html::invoke([=] {
            if (browser_ != nullptr) {
                browser_->GetHost()->CloseBrowser(true);
            }
        });
    }

    void remove()
    {
        close();
        removed_ = true;
    }

    bool is_removed() const { return removed_; }

  private:
    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        rect = CefRect(0, 0, format_desc_.square_width, format_desc_.square_height);
        return true;
    }

    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType      type,
                 const RectList&       dirtyRects,
                 const void*           buffer,
                 int                   width,
                 int                   height)
    {
        graph_->set_value("browser-tick-time", paint_timer_.elapsed() * format_desc_.fps * 0.5);
        paint_timer_.restart();
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        if (type != PET_VIEW)
            return;

        core::pixel_format_desc pixel_desc;
        pixel_desc.format = core::pixel_format::bgra;
        pixel_desc.planes.push_back(core::pixel_format_desc::plane(width, height, 4));

        auto frame = frame_factory_->create_frame(this, pixel_desc);
        std::memcpy(frame.image_data(0).begin(), buffer, width * height * 4);

        {
            std::lock_guard<std::mutex> lock(frames_mutex_);

            frames_.push(core::draw_frame(std::move(frame)));
            while (frames_.size() > 2) {
                frames_.pop();
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            }
        }
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        browser_ = browser;
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        removed_ = true;
        browser_ = nullptr;
    }

    bool DoClose(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        return false;
    }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override
    {
        loaded_ = true;
        execute_queued_javascript();
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                  CefProcessId                 source_process,
                                  CefRefPtr<CefProcessMessage> message) override
    {
        auto name = message->GetName().ToString();

        if (name == REMOVE_MESSAGE_NAME) {
            remove();

            return true;
        } else if (name == LOG_MESSAGE_NAME) {
            auto args     = message->GetArgumentList();
            auto severity = static_cast<boost::log::trivial::severity_level>(args->GetInt(0));
            auto msg      = args->GetString(1).ToWString();

            BOOST_LOG_SEV(log::logger::get(), severity) << print() << L" [renderer_process] " << msg;
        }

        return false;
    }

    void invoke_requested_animation_frames()
    {
        if (browser_)
            browser_->SendProcessMessage(CefProcessId::PID_RENDERER, CefProcessMessage::Create(TICK_MESSAGE_NAME));

        graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
        tick_timer_.restart();
    }

    bool try_pop(core::draw_frame& result)
    {
        std::lock_guard<std::mutex> lock(frames_mutex_);

        if (!frames_.empty()) {
            result = std::move(frames_.front());
            frames_.pop();

            return true;
        }

        return false;
    }

    core::draw_frame pop()
    {
        core::draw_frame frame;

        if (!try_pop(frame)) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(u8(print()) + " No frame in buffer"));
        }

        return frame;
    }

    void update()
    {
        invoke_requested_animation_frames();

        auto num_frames = [&] {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            return frames_.size();
        }();

        if (num_frames >= 1) {
            auto                        frame = pop();
            std::lock_guard<std::mutex> lock(last_frame_mutex_);
            last_frame_ = frame;
        } else {
            graph_->set_tag(diagnostics::tag_severity::SILENT, "late-frame");
        }
    }

    void do_execute_javascript(const std::wstring& javascript)
    {
        html::begin_invoke([=] {
            if (browser_ != nullptr)
                browser_->GetMainFrame()->ExecuteJavaScript(
                    u8(javascript).c_str(), browser_->GetMainFrame()->GetURL(), 0);
        });
    }

    void execute_queued_javascript()
    {
        std::wstring javascript;

        while (javascript_before_load_.try_pop(javascript))
            do_execute_javascript(javascript);
    }

    std::wstring print() const { return L"html[" + url_ + L"]"; }

    IMPLEMENT_REFCOUNTING(html_client);
};

class html_producer : public core::frame_producer_base
{
    core::video_format_desc             format_desc_;
    core::monitor::state                state_;
    const std::wstring                  url_;
    spl::shared_ptr<diagnostics::graph> graph_;

    CefRefPtr<html_client> client_;

  public:
    html_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                  const core::video_format_desc&              format_desc,
                  const std::wstring&                         url)
        : format_desc_(format_desc)
        , url_(url)
    {
        html::invoke([&] {
            client_ = new html_client(frame_factory, graph_, format_desc, url_);

            CefWindowInfo window_info;
            window_info.width                        = format_desc.square_width;
            window_info.height                       = format_desc.square_height;
            window_info.windowless_rendering_enabled = true;

            const bool enable_gpu = env::properties().get(L"configuration.html.enable-gpu", false);

            CefBrowserSettings browser_settings;
            browser_settings.web_security = cef_state_t::STATE_DISABLED;
            browser_settings.webgl        = enable_gpu ? cef_state_t::STATE_ENABLED : cef_state_t::STATE_DISABLED;
            double fps                    = format_desc.fps;
            browser_settings.windowless_frame_rate = int(ceil(fps));
            CefBrowserHost::CreateBrowser(window_info, client_.get(), url, browser_settings, nullptr);
        });
        state_["file/path"] = u8(url_);
    }

    ~html_producer()
    {
        if (client_)
            client_->close();
    }

    // frame_producer

    std::wstring name() const override { return L"html"; }

    core::draw_frame receive_impl(int nb_samples) override
    {
        if (client_) {
            if (client_->is_removed()) {
                client_ = nullptr;
                return core::draw_frame{};
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

    std::wstring print() const override { return L"html[" + url_ + L"]"; }

    const core::monitor::state& state() const { return state_; }
};

spl::shared_ptr<core::frame_producer> create_cg_producer(const core::frame_producer_dependencies& dependencies,
                                                         const std::vector<std::wstring>&         params)
{
    const auto html_prefix    = boost::iequals(params.at(0), L"[HTML]");
    const auto param_url      = html_prefix ? params.at(1) : params.at(0);
    const auto filename       = env::template_folder() + param_url + L".html";
    const auto found_filename = find_case_insensitive(filename);
    const auto http_prefix    = boost::algorithm::istarts_with(param_url, L"http:") ||
                             boost::algorithm::istarts_with(param_url, L"https:");

    if (!found_filename && !http_prefix && !html_prefix)
        return core::frame_producer::empty();

    const auto url = found_filename ? L"file://" + *found_filename : param_url;

    return core::create_destroy_proxy(
        spl::make_shared<html_producer>(dependencies.frame_factory, dependencies.format_desc, url));
}

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    return create_cg_producer(dependencies, params);
}

}} // namespace caspar::html
