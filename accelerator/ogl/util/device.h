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

#include <core/frame/frame.h>

#include <common/spl/memory.h>
#include <common/concurrency/executor.h>

namespace caspar { namespace accelerator { namespace ogl {

class texture;

class device sealed : public std::enable_shared_from_this<device>
{	
	device(const device&);
	device& operator=(const device&);

	executor executor_;
public:		

	// Static Members

	// Constructors

	device();
	~device();

	// Methods
		
	spl::shared_ptr<texture> create_texture(int width, int height, int stride);
	core::mutable_array		 create_array(int size);

	boost::unique_future<spl::shared_ptr<texture>> copy_async(const core::mutable_array& source, int width, int height, int stride);
	boost::unique_future<core::const_array>		   copy_async(const spl::shared_ptr<texture>& source);
	
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

	// Properties
	
	std::wstring version();

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}