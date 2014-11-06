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

#include "producer/html_producer.h"

#include <common/concurrency/executor.h>
#include <common/concurrency/future_util.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/thread/future.hpp>
#include <boost/lexical_cast.hpp>

#include <cef_app.h>

#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

namespace caspar { namespace html {

std::unique_ptr<executor> g_cef_executor;

void caspar_log(
		const CefRefPtr<CefBrowser>& browser,
		log::severity_level level,
		const std::string& message)
{
	if (browser)
	{
		auto msg = CefProcessMessage::Create(LOG_MESSAGE_NAME);
		msg->GetArgumentList()->SetInt(0, level);
		msg->GetArgumentList()->SetString(1, message);
		browser->SendProcessMessage(PID_BROWSER, msg);
	}
}

class animation_handler : public CefV8Handler
{
private:
	std::vector<CefRefPtr<CefV8Value>> callbacks_;
	boost::timer since_start_timer_;
public:
	CefRefPtr<CefBrowser> browser;
	std::function<CefRefPtr<CefV8Context>()> get_context;

	bool Execute(
			const CefString& name,
			CefRefPtr<CefV8Value> object,
			const CefV8ValueList& arguments,
			CefRefPtr<CefV8Value>& retval,
			CefString& exception) override
	{
		if (!CefCurrentlyOn(TID_RENDERER))
			return false;

		if (arguments.size() < 1 || !arguments.at(0)->IsFunction())
		{
			return false;
		}

		callbacks_.push_back(arguments.at(0));

		if (browser)
			browser->SendProcessMessage(PID_BROWSER, CefProcessMessage::Create(
					ANIMATION_FRAME_REQUESTED_MESSAGE_NAME));

		return true;
	}

	void tick()
	{
		if (!get_context)
			return;

		auto context = get_context();

		if (!context)
			return;

		if (!CefCurrentlyOn(TID_RENDERER))
			return;

		std::vector<CefRefPtr<CefV8Value>> callbacks;
		callbacks_.swap(callbacks);

		CefV8ValueList callback_args;
		CefTime timestamp;
		timestamp.Now();
		callback_args.push_back(CefV8Value::CreateDouble(
				since_start_timer_.elapsed() * 1000.0));

		BOOST_FOREACH(auto callback, callbacks)
		{
			callback->ExecuteFunctionWithContext(
					context, callback, callback_args);
		}
	}

	IMPLEMENT_REFCOUNTING(animation_handler);
};

class remove_handler : public CefV8Handler
{
	CefRefPtr<CefBrowser> browser_;
public:
	remove_handler(CefRefPtr<CefBrowser> browser)
		: browser_(browser)
	{
	}

	bool Execute(
			const CefString& name,
			CefRefPtr<CefV8Value> object,
			const CefV8ValueList& arguments,
			CefRefPtr<CefV8Value>& retval,
			CefString& exception) override
	{
		if (!CefCurrentlyOn(TID_RENDERER))
			return false;

		browser_->SendProcessMessage(
				PID_BROWSER,
				CefProcessMessage::Create(REMOVE_MESSAGE_NAME));

		return true;
	}

	IMPLEMENT_REFCOUNTING(remove_handler);
};

class renderer_application : public CefApp, CefRenderProcessHandler
{
	std::vector<std::pair<CefRefPtr<animation_handler>, CefRefPtr<CefV8Context>>> contexts_per_handlers_;
	//std::map<CefRefPtr<animation_handler>, CefRefPtr<CefV8Context>> contexts_per_handlers_;
public:
	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
	{
		return this;
	}

	CefRefPtr<CefV8Context> get_context(
			const CefRefPtr<animation_handler>& handler)
	{
		BOOST_FOREACH(auto& ctx, contexts_per_handlers_)
		{
			if (ctx.first == handler)
				return ctx.second;
		}

		return nullptr;
	}

	void OnContextCreated(
			CefRefPtr<CefBrowser> browser,
			CefRefPtr<CefFrame> frame,
			CefRefPtr<CefV8Context> context) override
	{
		caspar_log(browser, log::trace,
				"context for frame "
				+ boost::lexical_cast<std::string>(frame->GetIdentifier())
				+ " created");

		CefRefPtr<animation_handler> handler = new animation_handler;
		contexts_per_handlers_.push_back(std::make_pair(handler, context));
		auto handler_ptr = handler.get();

		handler->browser = browser;
		handler->get_context = [this, handler_ptr]
		{
			return get_context(handler_ptr);
		};

		auto window = context->GetGlobal();

		auto function = CefV8Value::CreateFunction(
				"requestAnimationFrame",
				handler.get());
		window->SetValue(
				"requestAnimationFrame",
				function,
				V8_PROPERTY_ATTRIBUTE_NONE);

		function = CefV8Value::CreateFunction(
				"remove",
				new remove_handler(browser));
		window->SetValue(
				"remove",
				function,
				V8_PROPERTY_ATTRIBUTE_NONE);
	}

	void OnContextReleased(
			CefRefPtr<CefBrowser> browser,
			CefRefPtr<CefFrame> frame,
			CefRefPtr<CefV8Context> context)
	{
		auto removed = boost::remove_if(
				contexts_per_handlers_, [&](const std::pair<
						CefRefPtr<animation_handler>,
						CefRefPtr<CefV8Context>>& c)
		{
			return c.second->IsSame(context);
		});

		if (removed != contexts_per_handlers_.end())
			caspar_log(browser, log::trace,
					"context for frame "
					+ boost::lexical_cast<std::string>(frame->GetIdentifier())
					+ " released");
		else
			caspar_log(browser, log::warning,
					"context for frame "
					+ boost::lexical_cast<std::string>(frame->GetIdentifier())
					+ " released, but not found");
	}

	void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override
	{
		contexts_per_handlers_.clear();
	}

	bool OnProcessMessageReceived(
			CefRefPtr<CefBrowser> browser,
			CefProcessId source_process,
			CefRefPtr<CefProcessMessage> message) override
	{
		if (message->GetName().ToString() == TICK_MESSAGE_NAME)
		{
			BOOST_FOREACH(auto& handler, contexts_per_handlers_)
			{
				handler.first->tick();
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	IMPLEMENT_REFCOUNTING(renderer_application);
};

bool init()
{
	CefMainArgs main_args;

	if (CefExecuteProcess(main_args, CefRefPtr<CefApp>(new renderer_application), nullptr) >= 0)
		return false;

	core::register_producer_factory(html::create_producer);
	
	g_cef_executor.reset(new executor(L"cef"));
	g_cef_executor->invoke([&]
	{
		CefSettings settings;
		//settings.windowless_rendering_enabled = true;
		CefInitialize(main_args, settings, nullptr, nullptr);
	});
	g_cef_executor->begin_invoke([&]
	{
		CefRunMessageLoop();
	});

	return true;
}

void uninit()
{
	invoke([]
	{
		CefQuitMessageLoop();
	});
	g_cef_executor->invoke([&]
	{
		CefShutdown();
	});
	g_cef_executor.reset();
}

class cef_task : public CefTask
{
private:
	boost::promise<void> promise_;
	std::function<void ()> function_;
public:
	cef_task(const std::function<void ()>& function)
		: function_(function)
	{
	}

	void Execute() override
	{
		CASPAR_LOG(trace) << "[cef_task] executing task";

		try
		{
			function_();
			promise_.set_value();
			CASPAR_LOG(trace) << "[cef_task] task succeeded";
		}
		catch (...)
		{
			promise_.set_exception(boost::current_exception());
			CASPAR_LOG(warning) << "[cef_task] task failed";
		}
	}

	boost::unique_future<void> future()
	{
		return promise_.get_future();
	}

	IMPLEMENT_REFCOUNTING(shutdown_task);
};

void invoke(const std::function<void()>& func)
{
	begin_invoke(func).get();
}


boost::unique_future<void> begin_invoke(const std::function<void()>& func)
{
	CefRefPtr<cef_task> task = new cef_task(func);

	if (CefCurrentlyOn(TID_UI))
	{
		// Avoid deadlock.
		task->Execute();
		return task->future();
	}

	if (CefPostTask(TID_UI, task.get()))
		return task->future();
	else
		BOOST_THROW_EXCEPTION(caspar_exception()
				<< msg_info("[cef_executor] Could not post task"));
}
}}
