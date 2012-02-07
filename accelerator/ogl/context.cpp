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

// TODO: Smart GC

#include "../stdafx.h"

#include "context.h"

#include "shader.h"

#include <common/assert.h>
#include <common/except.h>
#include <common/gl/gl_check.h>

#include <boost/foreach.hpp>

#include <gl/glew.h>

#include <SFML/Window/Context.hpp>

#include <array>
#include <unordered_map>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

namespace caspar { namespace accelerator { namespace ogl {
	
struct context::impl : public std::enable_shared_from_this<impl>
{
	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<std::shared_ptr<device_buffer>>>, 4>	device_pools_;
	std::array<tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<std::shared_ptr<host_buffer>>>, 2>		host_pools_;
	
	GLuint fbo_;

	executor& executor_;
				
	impl(executor& executor) 
		: executor_(executor)
	{
		CASPAR_LOG(info) << L"Initializing OpenGL Device.";
		
		executor_.invoke([=]
		{
			context_.reset(new sf::Context());
			context_->SetActive(true);
		
			if (glewInit() != GLEW_OK)
				BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
						
			CASPAR_LOG(info) << L"OpenGL " << version();

			if(!GLEW_VERSION_3_0)
				BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Your graphics card does not meet the minimum hardware requirements since it does not support OpenGL 3.0 or higher. CasparCG Server will not be able to continue."));
	
			glGenFramebuffers(1, &fbo_);				
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

			CASPAR_LOG(info) << L"Successfully initialized OpenGL Device.";
		});
	}

	~impl()
	{
		executor_.invoke([=]
		{
			BOOST_FOREACH(auto& pool, device_pools_)
				pool.clear();
			BOOST_FOREACH(auto& pool, host_pools_)
				pool.clear();
			glDeleteFramebuffers(1, &fbo_);
		});
	}

	spl::shared_ptr<device_buffer> allocate_device_buffer(int width, int height, int stride)
	{
		std::shared_ptr<device_buffer> buffer;
		try
		{
			buffer.reset(new device_buffer(width, height, stride));
		}
		catch(...)
		{
			CASPAR_LOG(error) << L"ogl: create_device_buffer failed!";
			throw;
		}
		return spl::make_shared_ptr(buffer);
	}
				
	spl::shared_ptr<device_buffer> create_device_buffer(int width, int height, int stride)
	{
		CASPAR_VERIFY(stride > 0 && stride < 5);
		CASPAR_VERIFY(width > 0 && height > 0);
		
		auto pool = &device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
		
		std::shared_ptr<device_buffer> buffer;
		if(!pool->try_pop(buffer))		
			buffer = executor_.invoke([&]{return allocate_device_buffer(width, height, stride);}, task_priority::high_priority);			
	
		auto self = shared_from_this();
		return spl::shared_ptr<device_buffer>(buffer.get(), [self, buffer, pool](device_buffer*) mutable
		{		
			pool->push(buffer);	
		});
	}

	spl::shared_ptr<host_buffer> allocate_host_buffer(int size, host_buffer::usage usage)
	{
		std::shared_ptr<host_buffer> buffer;

		try
		{
			buffer.reset(new host_buffer(size, usage));
			if(usage == host_buffer::usage::write_only)
				buffer->map();
			else
				buffer->unmap();			
		}
		catch(...)
		{
			CASPAR_LOG(error) << L"ogl: create_host_buffer failed!";
			throw;	
		}

		return spl::make_shared_ptr(buffer);
	}
	
	spl::shared_ptr<host_buffer> create_host_buffer(int size, host_buffer::usage usage)
	{
		CASPAR_VERIFY(usage == host_buffer::usage::write_only || usage == host_buffer::usage::read_only);
		CASPAR_VERIFY(size > 0);
		
		auto pool = &host_pools_[usage.value()][size];
		
		std::shared_ptr<host_buffer> buffer;
		if(!pool->try_pop(buffer))	
			buffer = executor_.invoke([=]{return allocate_host_buffer(size, usage);}, task_priority::high_priority);	
	
		auto self		= shared_from_this();
		bool is_write	= (usage == host_buffer::usage::write_only);
		return spl::shared_ptr<host_buffer>(buffer.get(), [self, is_write, buffer, pool](host_buffer*) mutable
		{
			self->executor_.begin_invoke([=]() mutable
			{		
				if(is_write)
					buffer->map();
				else
					buffer->unmap();

				pool->push(buffer);
			}, task_priority::high_priority);	
		});
	}
		
	std::wstring version()
	{	
		static std::wstring ver = L"Not found";
		try
		{
			ver = u16(executor_.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));})
			+ " "	+ executor_.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));}));			
		}
		catch(...){}

		return ver;
	}
			
	boost::unique_future<spl::shared_ptr<device_buffer>> copy_async(spl::shared_ptr<host_buffer>& source, int width, int height, int stride)
	{
		return executor_.begin_invoke([=]() -> spl::shared_ptr<device_buffer>
		{
			auto result = create_device_buffer(width, height, stride);
			result->copy_from(*source);
			return result;
		}, task_priority::high_priority);
	}

	void yield()
	{
		executor_.yield(task_priority::high_priority);
	}
};

context::context() 
	: executor_(L"context")
	, impl_(new impl(executor_))
{
}
	
void													context::yield(){impl_->yield();}	
spl::shared_ptr<device_buffer>							context::create_device_buffer(int width, int height, int stride){return impl_->create_device_buffer(width, height, stride);}
spl::shared_ptr<host_buffer>							context::create_host_buffer(int size, host_buffer::usage usage){return impl_->create_host_buffer(size, usage);}
boost::unique_future<spl::shared_ptr<device_buffer>>	context::copy_async(spl::shared_ptr<host_buffer>& source, int width, int height, int stride){return impl_->copy_async(source, width, height, stride);}
std::wstring											context::version(){return impl_->version();}


}}}


