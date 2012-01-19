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

#include <common/concurrency/executor.h>
#include <common/memory/safe_ptr.h>

#include <gl/glew.h>

#include <SFML/Window/Context.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <array>
#include <unordered_map>

namespace caspar { namespace core {

class shader;

template<typename T>
struct buffer_pool
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

class ogl_device : public std::enable_shared_from_this<ogl_device>, boost::noncopyable
{	
	std::unordered_map<GLenum, bool> caps_;
	std::array<size_t, 4>			 viewport_;
	std::array<size_t, 4>			 scissor_;
	const GLubyte*					 pattern_;
	GLint							 attached_texture_;
	GLuint							 attached_fbo_;
	GLint							 active_shader_;
	std::array<GLint, 16>			 binded_textures_;
	std::array<GLint, 4>			 blend_func_;
	GLenum							 read_buffer_;

	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<size_t, safe_ptr<buffer_pool<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<size_t, safe_ptr<buffer_pool<host_buffer>>>, 2> host_pools_;
	
	GLuint fbo_;

	executor executor_;
				
	ogl_device();
public:		
	static safe_ptr<ogl_device> create();
	~ogl_device();

	// Not thread-safe, must be called inside of context
	void enable(GLenum cap);
	void disable(GLenum cap);
	void viewport(size_t x, size_t y, size_t width, size_t height);
	void scissor(size_t x, size_t y, size_t width, size_t height);
	void stipple_pattern(const GLubyte* pattern);

	void attach(device_buffer& texture);
	void clear(device_buffer& texture);
	
	void blend_func(int c1, int c2, int a1, int a2);
	void blend_func(int c1, int c2);
	
	void use(shader& shader);

	void read_buffer(device_buffer& texture);

	void flush();

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
		
	safe_ptr<device_buffer> create_device_buffer(size_t width, size_t height, size_t stride);
	safe_ptr<host_buffer> create_host_buffer(size_t size, host_buffer::usage_t usage);
	
	void yield();
	boost::unique_future<void> gc();

	std::wstring version();

private:
	safe_ptr<device_buffer> allocate_device_buffer(size_t width, size_t height, size_t stride);
	safe_ptr<host_buffer> allocate_host_buffer(size_t size, host_buffer::usage_t usage);
};

}}