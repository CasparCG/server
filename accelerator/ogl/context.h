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

#pragma once

#include "host_buffer.h"
#include "device_buffer.h"

#include <common/spl/memory.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <common/concurrency/executor.h>

namespace caspar { namespace accelerator { namespace ogl {
	
class context : public std::enable_shared_from_this<context>, boost::noncopyable
{	
	executor executor_;
public:		
	context();
	
	void yield();
	
	spl::shared_ptr<device_buffer>							create_device_buffer(int width, int height, int stride);
	spl::shared_ptr<host_buffer>							create_host_buffer(int size, host_buffer::usage usage);

	boost::unique_future<spl::shared_ptr<device_buffer>>	copy_async(spl::shared_ptr<host_buffer>& source, int width, int height, int stride);
	
	std::wstring version();

	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{			
		return executor_.begin_invoke(std::forward<Func>(func), priority);
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> decltype(func())
	{
		return executor_.invoke(std::forward<Func>(func), priority);
	}
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}