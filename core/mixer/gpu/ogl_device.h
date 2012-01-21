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
#include <stack>
#include <unordered_map>
#include <type_traits>

FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {

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

class ogl_device : public std::enable_shared_from_this<ogl_device>
				 , boost::noncopyable
{	
	__declspec(align(16)) struct state
	{
		std::array<GLubyte, 32*32> 				pattern;
		std::array<GLint, 16>					binded_textures;
		std::array<GLint, 4>					viewport;
		std::array<GLint, 4>					scissor;
		std::array<GLint, 4>					blend_func;
		GLint									attached_texture;
		GLint									active_shader;
		GLint padding[2];

		state(); 
		state(const state& other);
		state& operator=(const state& other);
	};
	
	state state_;
	std::stack<state> state_stack_;

	std::map<GLenum, bool> caps_;
	void enable(GLenum cap);
	void disable(GLenum cap);

	std::unique_ptr<sf::Context> context_;
	GLuint fbo_;
	
	std::array<tbb::concurrent_unordered_map<int, safe_ptr<buffer_pool<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<int, safe_ptr<buffer_pool<host_buffer>>>, 2> host_pools_;
	
	executor executor_;
				
	ogl_device();

	void use(GLint id);
	void attach(GLint id);
	void bind(GLint id, int index);	
	void flush();
	
	friend class scoped_state;
public:		
	void push_state();
	state pop_state();
	
	static safe_ptr<ogl_device> create();
	~ogl_device();

	// Not thread-safe, must be called inside of context
	void viewport(int x, int y, int width, int height);
	void viewport(const std::array<GLint, 4>& ar);
	void scissor(int x, int y, int width, int height);
	void scissor(const std::array<GLint, 4>& ar);
	void stipple_pattern(const std::array<GLubyte, 32*32>& pattern);
		
	void blend_func(const std::array<GLint, 4>& ar);
	void blend_func(int c1, int c2, int a1, int a2);
	void blend_func(int c1, int c2);
	
	void use(const shader& shader);

	void attach(const device_buffer& texture);

	void bind(const device_buffer& texture, int index);


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
		
	safe_ptr<device_buffer> create_device_buffer(int width, int height, int stride, bool zero = false);
	safe_ptr<host_buffer> create_host_buffer(int size, host_buffer::usage_t usage);

	boost::unique_future<safe_ptr<host_buffer>> transfer(const safe_ptr<device_buffer>& source);
	boost::unique_future<safe_ptr<device_buffer>> transfer(const safe_ptr<host_buffer>& source, int width, int height, int stride);
	
	bool yield();
	boost::unique_future<void> gc();

	std::wstring version();

private:
	safe_ptr<device_buffer> allocate_device_buffer(int width, int height, int stride);
	safe_ptr<host_buffer> allocate_host_buffer(int size, host_buffer::usage_t usage);
};

class scoped_state
{
	ogl_device& context_;
public:
	scoped_state(ogl_device& context)
		: context_(context)
	{
		context_.push_state();
	}

	~scoped_state()
	{
		context_.pop_state();
	}
};

}}