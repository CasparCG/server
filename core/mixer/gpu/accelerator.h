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

#include <common/forward.h>
#include <common/concurrency/executor.h>
#include <common/memory/safe_ptr.h>

#include <gl/glew.h>

#include <SFML/Window/Context.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <boost/noncopyable.hpp>

#include <array>
#include <unordered_map>

FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core { namespace gpu {

class shader;

template<typename T>
struct buffer_pool : boost::noncopyable
{
	tbb::atomic<int> usage_count;
	tbb::atomic<int> flush_count;
	tbb::concurrent_bounded_queue<std::shared_ptr<T>> items;

	buffer_pool()
	{
		usage_count = 0;
		flush_count = 0;
	}
};

class accelerator : public std::enable_shared_from_this<accelerator>
				 , boost::noncopyable
{	
	std::array<GLubyte, 32*32>		 pattern_;
	std::array<int, 4>				 viewport_;
	std::array<int, 4>				 scissor_;
	std::array<GLint, 4>			 blend_func_;
	GLint							 attached_texture_;
	GLuint							 attached_fbo_;
	GLint							 active_shader_;
	
	std::unordered_map<GLenum, bool> caps_;
	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<int, safe_ptr<buffer_pool<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<int, safe_ptr<buffer_pool<host_buffer>>>, 2> host_pools_;
	
	GLuint fbo_;

	executor executor_;
				
	accelerator();
public:		
	static safe_ptr<accelerator> create();
	~accelerator();

	// Not thread-safe, must be called inside of context
	void enable(GLenum cap);
	void disable(GLenum cap);
	void viewport(int x, int y, int width, int height);
	void scissor(int x, int y, int width, int height);
	void stipple_pattern(const std::array<GLubyte, 32*32>& pattern);

	void attach(device_buffer& texture);
	void clear(device_buffer& texture);
	
	void blend_func(int c1, int c2, int a1, int a2);
	void blend_func(int c1, int c2);
	
	void use(shader& shader);
		
	// thread-afe
	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{			
		return executor_.begin_invoke(std::forward<Func>(func), priority);
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority priority = normal_priority) -> decltype(func())
	{
		return executor_.invoke(std::forward<Func>(func), priority);
	}
		
	safe_ptr<device_buffer> create_device_buffer(int width, int height, int stride);
	safe_ptr<host_buffer> create_host_buffer(int size, host_buffer::usage usage);
	
	boost::unique_future<void> gc();

	std::wstring version();

	boost::unique_future<safe_ptr<device_buffer>> copy_async(safe_ptr<host_buffer>&& source, int width, int height, int stride);

private:
	safe_ptr<device_buffer> allocate_device_buffer(int width, int height, int stride);
	safe_ptr<host_buffer> allocate_host_buffer(int size, host_buffer::usage usage);
};

}}}