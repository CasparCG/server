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

#include <common/except.h>
#include <common/assert.h>
#include <common/gl/gl_check.h>

#include <boost/foreach.hpp>

#include <gl/glew.h>

namespace caspar { namespace accelerator { namespace ogl {

context::context() 
	: executor_(L"context")
{
	CASPAR_LOG(info) << L"Initializing OpenGL Device.";
		
	invoke([=]
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
		
		if (glewInit() != GLEW_OK)
			BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
						
		CASPAR_LOG(info) << L"OpenGL " << version();

		if(!GLEW_VERSION_3_0)
			BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Your graphics card does not meet the minimum hardware requirements since it does not support OpenGL 3.0 or higher. CasparCG Server will not be able to continue."));
	
		glGenFramebuffers(1, &fbo_);	
		
		CASPAR_LOG(info) << L"Successfully initialized OpenGL Device.";
	});
}

context::~context()
{
	invoke([=]
	{
		BOOST_FOREACH(auto& pool, device_pools_)
			pool.clear();
		BOOST_FOREACH(auto& pool, host_pools_)
			pool.clear();
		glDeleteFramebuffers(1, &fbo_);
	});
}

spl::shared_ptr<device_buffer> context::allocate_device_buffer(int width, int height, int stride)
{
	std::shared_ptr<device_buffer> buffer;
	try
	{
		buffer.reset(new device_buffer(shared_from_this(), width, height, stride));
	}
	catch(...)
	{
		try
		{
			executor_.yield();
			gc().wait();
					
			// Try again
			buffer.reset(new device_buffer(shared_from_this(), width, height, stride));
		}
		catch(...)
		{
			CASPAR_LOG(error) << L"ogl: create_device_buffer failed!";
			throw;
		}
	}
	return spl::make_shared_ptr(buffer);
}
				
spl::shared_ptr<device_buffer> context::create_device_buffer(int width, int height, int stride)
{
	CASPAR_VERIFY(stride > 0 && stride < 5);
	CASPAR_VERIFY(width > 0 && height > 0);
	auto& pool = device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
	std::shared_ptr<device_buffer> buffer;
	if(!pool->items.try_pop(buffer))		
		buffer = executor_.invoke([&]{return allocate_device_buffer(width, height, stride);}, task_priority::high_priority);			
	
	//++pool->usage_count;

	return spl::shared_ptr<device_buffer>(buffer.get(), [=](device_buffer*) mutable
	{		
		pool->items.push(buffer);	
	});
}

spl::shared_ptr<host_buffer> context::allocate_host_buffer(int size, host_buffer::usage usage)
{
	std::shared_ptr<host_buffer> buffer;

	try
	{
		buffer.reset(new host_buffer(shared_from_this(), size, usage));
		if(usage == host_buffer::usage::write_only)
			buffer->map();
		else
			buffer->unmap();			
	}
	catch(...)
	{
		try
		{
			executor_.yield();
			gc().wait();

			// Try again
			buffer.reset(new host_buffer(shared_from_this(), size, usage));
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
	}

	return spl::make_shared_ptr(buffer);
}
	
spl::shared_ptr<host_buffer> context::create_host_buffer(int size, host_buffer::usage usage)
{
	CASPAR_VERIFY(usage == host_buffer::usage::write_only || usage == host_buffer::usage::read_only);
	CASPAR_VERIFY(size > 0);
	auto& pool = host_pools_[usage.value()][size];
	std::shared_ptr<host_buffer> buffer;
	if(!pool->items.try_pop(buffer))	
		buffer = executor_.invoke([=]{return allocate_host_buffer(size, usage);}, task_priority::high_priority);	
	
	auto self = shared_from_this();
	bool is_write_only	= (usage == host_buffer::usage::write_only);
	return spl::shared_ptr<host_buffer>(buffer.get(), [=](host_buffer*) mutable
	{
		self->executor_.begin_invoke([=]() mutable
		{		
			if(is_write_only)
				buffer->map();
			else
				buffer->unmap();

			pool->items.push(buffer);
		}, task_priority::high_priority);	
	});
}

spl::shared_ptr<context> context::create()
{
	return spl::shared_ptr<context>(new context());
}

//template<typename T>
//void flush_pool(buffer_pool<T>& pool)
//{	
//	if(pool.flush_count.fetch_and_increment() < 16)
//		return;
//
//	if(pool.usage_count.fetch_and_store(0) < pool.items.size())
//	{
//		std::shared_ptr<T> buffer;
//		pool.items.try_pop(buffer);
//	}
//
//	pool.flush_count = 0;
//	pool.usage_count = 0;
//}

boost::unique_future<void> context::gc()
{	
	return begin_invoke([=]
	{
		CASPAR_LOG(info) << " ogl: Running GC.";		
	
		try
		{
			BOOST_FOREACH(auto& pools, device_pools_)
			{
				BOOST_FOREACH(auto& pool, pools)
					pool.second->items.clear();
			}
			BOOST_FOREACH(auto& pools, host_pools_)
			{
				BOOST_FOREACH(auto& pool, pools)
					pool.second->items.clear();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}, task_priority::high_priority);
}

std::wstring context::version()
{	
	static std::wstring ver = L"Not found";
	try
	{
		ver = u16(invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));})
		+ " "	+ invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));}));			
	}
	catch(...){}

	return ver;
}

void context::attach(device_buffer& texture)
{	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, texture.id(), 0));
}

void context::clear(device_buffer& texture)
{	
	attach(texture);
	GL(glClear(GL_COLOR_BUFFER_BIT));
}

void context::use(shader& shader)
{	
	GL(glUseProgramObjectARB(shader.id()));	
}

boost::unique_future<spl::shared_ptr<device_buffer>> context::copy_async(spl::shared_ptr<host_buffer>& source, int width, int height, int stride)
{
	return executor_.begin_invoke([=]() -> spl::shared_ptr<device_buffer>
	{
		auto result = create_device_buffer(width, height, stride);
		result->copy_from(source);
		return result;
	}, task_priority::high_priority);
}

void context::yield()
{
	executor_.yield(task_priority::high_priority);
}

}}}


