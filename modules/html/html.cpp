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

#include <berkelium/Berkelium.hpp>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace caspar { namespace html {
	
std::unique_ptr<executor> g_berkelium_executor;
std::unique_ptr<executor> g_berkelium_timer_executor;

void tick()
{
	g_berkelium_timer_executor->begin_invoke([&]
	{
		static boost::asio::io_service io;

		boost::asio::deadline_timer t(io, boost::posix_time::milliseconds(10));

		g_berkelium_executor->invoke([&]
		{
			Berkelium::update();
		});

		t.wait();

		tick();
	});
}

void init()
{
	core::register_producer_factory(html::create_producer);

	g_berkelium_executor.reset(new executor(L"berkelium"));
	g_berkelium_timer_executor.reset(new executor(L"g_berkelium_timer"));
	
	g_berkelium_executor->invoke([&]
	{
		if (!Berkelium::init(Berkelium::FileString::empty()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to initialize Berkelium"));
	});
	
	g_berkelium_timer_executor->begin_invoke([&]
	{
		tick();
	});
}

void invoke(const std::function<void()>& func)
{
	g_berkelium_executor->invoke(func);
}

void uninit()
{
	g_berkelium_timer_executor.reset();

	g_berkelium_executor->invoke([&]
	{
		Berkelium::destroy();
	});

	g_berkelium_executor.reset();
}

}}
