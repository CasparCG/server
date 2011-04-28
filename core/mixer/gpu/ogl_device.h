/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "host_buffer.h"
#include "device_buffer.h"

#include <common/concurrency/executor.h>
#include <common/memory/safe_ptr.h>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread/future.hpp>

#include <array>

#include <SFML/Window/Context.hpp>

namespace caspar { namespace mixer {

class ogl_device
{	
	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<host_buffer>>>, 2> host_pools_;

	executor executor_;

	ogl_device();
public:	
	virtual ~ogl_device();

	static safe_ptr<ogl_device> create()
	{
		static safe_ptr<ogl_device> instance(new ogl_device()); // Use the same ogl-device for all channels inorder to ensure that frames are always valid for all "context".
		return instance;
	}

	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
	{			
		return executor_.begin_invoke(std::forward<Func>(func));
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		return executor_.invoke(std::forward<Func>(func));
	}
		
	safe_ptr<device_buffer> create_device_buffer(size_t width, size_t height, size_t stride);
	safe_ptr<host_buffer> create_host_buffer(size_t size, host_buffer::usage_t usage);
};

}}