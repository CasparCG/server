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

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cef_app.h>

#include <tbb/atomic.h>

#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

namespace caspar { namespace html {

tbb::atomic<bool>		  g_cef_running;
std::unique_ptr<executor> g_cef_executor;

void tick()
{
	if (!g_cef_running)
		return;

	CefDoMessageLoopWork();
	g_cef_executor->begin_invoke([&]{ tick(); });
}

bool init()
{
	CefMainArgs main_args;

	if (CefExecuteProcess(main_args, nullptr, nullptr) >= 0)
		return false;

	core::register_producer_factory(html::create_producer);
	
	g_cef_executor.reset(new executor(L"cef"));
	g_cef_running = true;

	g_cef_executor->invoke([&]
	{
		CefSettings settings;
		//settings.windowless_rendering_enabled = true;
		CefInitialize(main_args, settings, nullptr, nullptr);
	});

	g_cef_executor->begin_invoke([&]
	{
		tick();
	});

	return true;
}

void uninit()
{
	g_cef_executor->invoke([=]
	{
		g_cef_running = false;
		g_cef_executor->wait();
		CefShutdown();
	});
}


void invoke(const std::function<void()>& func)
{
	g_cef_executor->invoke(func);
}


void begin_invoke(const std::function<void()>& func)
{
	g_cef_executor->begin_invoke(func);
}
}}
