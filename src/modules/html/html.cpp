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

#include "html.h"

#include "producer/html_cg_proxy.h"
#include "producer/html_producer.h"

#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>

#include <core/producer/cg_proxy.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm/remove_if.hpp>

#include <memory>
#include <utility>

#pragma warning(push)
#pragma warning(disable : 4458)
#include <include/cef_app.h>
#include <include/cef_version.h>
#pragma warning(pop)

namespace caspar { namespace html {

std::unique_ptr<executor> g_cef_executor;

void caspar_log(const CefRefPtr<CefBrowser>&        browser,
                boost::log::trivial::severity_level level,
                const std::string&                  message)
{
    if (browser != nullptr) {
        auto msg = CefProcessMessage::Create(LOG_MESSAGE_NAME);
        msg->GetArgumentList()->SetInt(0, level);
        msg->GetArgumentList()->SetString(1, message);

        CefRefPtr<CefFrame> mainFrame = browser->GetMainFrame();
        if (mainFrame) {
            mainFrame->SendProcessMessage(PID_BROWSER, msg);
        }
    }
}

class remove_handler : public CefV8Handler
{
    CefRefPtr<CefBrowser> browser_;

  public:
    explicit remove_handler(const CefRefPtr<CefBrowser>& browser)
        : browser_(browser)
    {
    }

    bool Execute(const CefString&       name,
                 CefRefPtr<CefV8Value>  object,
                 const CefV8ValueList&  arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString&             exception) override
    {
        if (!CefCurrentlyOn(TID_RENDERER)) {
            return false;
        }

        CefRefPtr<CefFrame> mainFrame = browser_->GetMainFrame();
        if (mainFrame) {
            mainFrame->SendProcessMessage(PID_BROWSER, CefProcessMessage::Create(REMOVE_MESSAGE_NAME));
        }

        return true;
    }

    IMPLEMENT_REFCOUNTING(remove_handler);
};

class renderer_application
    : public CefApp
    , CefRenderProcessHandler
{
    std::vector<CefRefPtr<CefV8Context>> contexts_;
    const bool                           enable_gpu_;

  public:
    explicit renderer_application(const bool enable_gpu)
        : enable_gpu_(enable_gpu)
    {
    }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    void
    OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override
    {
        caspar_log(browser,
                   boost::log::trivial::trace,
                   "context for frame " + std::to_string(frame->GetIdentifier()) + " created");
        contexts_.push_back(context);

        auto window = context->GetGlobal();

        window->SetValue(
            "remove", CefV8Value::CreateFunction("remove", new remove_handler(browser)), V8_PROPERTY_ATTRIBUTE_NONE);

        CefRefPtr<CefV8Value>     ret;
        CefRefPtr<CefV8Exception> exception;
        bool                      injected = context->Eval(R"(
            window.caspar = window.casparcg = {};
		)",
                                      CefString(),
                                      1,
                                      ret,
                                      exception);

        if (!injected) {
            caspar_log(browser, boost::log::trivial::error, "Could not inject javascript animation code.");
        }
    }

    void OnContextReleased(CefRefPtr<CefBrowser>   browser,
                           CefRefPtr<CefFrame>     frame,
                           CefRefPtr<CefV8Context> context) override
    {
        auto removed =
            boost::remove_if(contexts_, [&](const CefRefPtr<CefV8Context>& c) { return c->IsSame(context); });

        if (removed != contexts_.end()) {
            caspar_log(browser,
                       boost::log::trivial::trace,
                       "context for frame " + std::to_string(frame->GetIdentifier()) + " released");
        } else {
            caspar_log(browser,
                       boost::log::trivial::warning,
                       "context for frame " + std::to_string(frame->GetIdentifier()) + " released, but not found");
        }
    }

    void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override { contexts_.clear(); }

    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override
    {
        if (enable_gpu_) {
            command_line->AppendSwitch("enable-webgl");

            // This gives better performance on the gpu->cpu readback, but can perform worse with intense templates
            auto backend = env::properties().get(L"configuration.html.angle-backend", L"gl");
            if (backend.size() > 0) {
                command_line->AppendSwitchWithValue("use-angle", backend);
            }
        }

        command_line->AppendSwitch("disable-web-security");
        command_line->AppendSwitch("enable-begin-frame-scheduling");
        command_line->AppendSwitch("enable-media-stream");
        command_line->AppendSwitch("use-fake-ui-for-media-stream");
        command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
        command_line->AppendSwitchWithValue("remote-allow-origins", "*");

        if (process_type.empty() && !enable_gpu_) {
            // This gives more performance, but disabled gpu effects. Without it a single 1080p producer cannot be run
            // smoothly

            command_line->AppendSwitch("disable-gpu");
            command_line->AppendSwitch("disable-gpu-compositing");
            command_line->AppendSwitchWithValue("disable-gpu-vsync", "gpu");
        }
    }

    IMPLEMENT_REFCOUNTING(renderer_application);
};

bool intercept_command_line(int argc, char** argv)
{
#ifdef _WIN32
    CefMainArgs main_args;
#else
    CefMainArgs main_args(argc, argv);
#endif

    return CefExecuteProcess(main_args, CefRefPtr<CefApp>(new renderer_application(false)), nullptr) >= 0;
}

void init(const core::module_dependencies& dependencies)
{
    dependencies.producer_registry->register_producer_factory(L"HTML Producer", html::create_producer);

    CefMainArgs main_args;
    g_cef_executor = std::make_unique<executor>(L"cef");
    g_cef_executor->invoke([&] {
#ifdef WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
        const bool enable_gpu = env::properties().get(L"configuration.html.enable-gpu", false);

        CefSettings settings;
        settings.command_line_args_disabled   = false;
        settings.no_sandbox                   = true;
        settings.remote_debugging_port        = env::properties().get(L"configuration.html.remote-debugging-port", 0);
        settings.windowless_rendering_enabled = true;
        CefInitialize(main_args, settings, CefRefPtr<CefApp>(new renderer_application(enable_gpu)), nullptr);
    });
    g_cef_executor->begin_invoke([&] { CefRunMessageLoop(); });
    dependencies.cg_registry->register_cg_producer(
        L"html",
        {L".html"},
        [](const spl::shared_ptr<core::frame_producer>& producer) { return spl::make_shared<html_cg_proxy>(producer); },
        [](const core::frame_producer_dependencies& dependencies, const std::wstring& filename) {
            return html::create_cg_producer(dependencies, {filename});
        },
        false);

    auto cef_version_major = std::to_wstring(cef_version_info(0));
    auto cef_revision      = std::to_wstring(cef_version_info(1));
    auto chrome_major      = std::to_wstring(cef_version_info(2));
    auto chrome_minor      = std::to_wstring(cef_version_info(3));
    auto chrome_build      = std::to_wstring(cef_version_info(4));
    auto chrome_patch      = std::to_wstring(cef_version_info(5));
}

void uninit()
{
    invoke([] { CefQuitMessageLoop(); });
    g_cef_executor->begin_invoke([&] { CefShutdown(); });
    g_cef_executor.reset();
}

class cef_task : public CefTask
{
  private:
    std::promise<void>    promise_;
    std::function<void()> function_;

  public:
    explicit cef_task(std::function<void()> function)
        : function_(std::move(function))
    {
    }

    void Execute() override
    {
        CASPAR_LOG(trace) << "[cef_task] executing task";

        try {
            function_();
            promise_.set_value();
            CASPAR_LOG(trace) << "[cef_task] task succeeded";
        } catch (...) {
            promise_.set_exception(std::current_exception());
            CASPAR_LOG(warning) << "[cef_task] task failed";
        }
    }

    std::future<void> future() { return promise_.get_future(); }

    IMPLEMENT_REFCOUNTING(cef_task);
};

void invoke(const std::function<void()>& func) { begin_invoke(func).get(); }

std::future<void> begin_invoke(const std::function<void()>& func)
{
    CefRefPtr<cef_task> task = new cef_task(func);

    if (CefCurrentlyOn(TID_UI)) {
        // Avoid deadlock.
        task->Execute();
        return task->future();
    }

    if (CefPostTask(TID_UI, task.get())) {
        return task->future();
    }
    CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("[cef_executor] Could not post task"));
}

}} // namespace caspar::html
