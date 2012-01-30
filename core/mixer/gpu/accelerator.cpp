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

#include "../../stdafx.h"

#include "accelerator.h"

#include "shader.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <common/assert.h>
#include <boost/foreach.hpp>

#include <gl/glew.h>

namespace caspar { namespace core { namespace gpu {

accelerator::accelerator() 
	: executor_(L"accelerator")
	, attached_texture_(0)
	, attached_fbo_(0)
	, active_shader_(0)
{
	CASPAR_LOG(info) << L"Initializing OpenGL Device.";

	viewport_.assign(std::numeric_limits<GLint>::max());
	scissor_.assign(std::numeric_limits<GLint>::max());
	blend_func_.assign(std::numeric_limits<GLint>::max());
	pattern_.assign(0xFF);
		
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
				
		enable(GL_TEXTURE_2D);
	});
}

accelerator::~accelerator()
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

safe_ptr<device_buffer> accelerator::allocate_device_buffer(int width, int height, int stride)
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
	return make_safe_ptr(buffer);
}
				
safe_ptr<device_buffer> accelerator::create_device_buffer(int width, int height, int stride)
{
	CASPAR_VERIFY(stride > 0 && stride < 5);
	CASPAR_VERIFY(width > 0 && height > 0);
	auto& pool = device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
	std::shared_ptr<device_buffer> buffer;
	if(!pool->items.try_pop(buffer))		
		buffer = executor_.invoke([&]{return allocate_device_buffer(width, height, stride);}, high_priority);			
	
	//++pool->usage_count;

	return safe_ptr<device_buffer>(buffer.get(), [=](device_buffer*) mutable
	{		
		pool->items.push(buffer);	
	});
}

safe_ptr<host_buffer> accelerator::allocate_host_buffer(int size, host_buffer::usage usage)
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

	return make_safe_ptr(buffer);
}
	
safe_ptr<host_buffer> accelerator::create_host_buffer(int size, host_buffer::usage usage)
{
	CASPAR_VERIFY(usage == host_buffer::usage::write_only || usage == host_buffer::usage::read_only);
	CASPAR_VERIFY(size > 0);
	auto& pool = host_pools_[usage.value()][size];
	std::shared_ptr<host_buffer> buffer;
	if(!pool->items.try_pop(buffer))	
		buffer = executor_.invoke([=]{return allocate_host_buffer(size, usage);}, high_priority);	
	
	//++pool->usage_count;

	auto self			= shared_from_this();
	bool is_write_only	= (usage == host_buffer::usage::write_only);

	return safe_ptr<host_buffer>(buffer.get(), [=](host_buffer*) mutable
	{
		self->executor_.begin_invoke([=]() mutable
		{		
			if(is_write_only)
				buffer->map();
			else
				buffer->unmap();

			pool->items.push(buffer);
		}, high_priority);	
	});
}

safe_ptr<accelerator> accelerator::create()
{
	return safe_ptr<accelerator>(new accelerator());
}

boost::unique_future<void> accelerator::gc()
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
	}, high_priority);
}

std::wstring accelerator::version()
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


void accelerator::enable(GLenum cap)
{
	auto& val = caps_[cap];
	if(!val)
	{
		glEnable(cap);
		val = true;
	}
}

void accelerator::disable(GLenum cap)
{
	auto& val = caps_[cap];
	if(val)
	{
		glDisable(cap);
		val = false;
	}
}

void accelerator::viewport(int x, int y, int width, int height)
{
	std::array<GLint, 4> viewport = {{x, y, width, height}};
	if(viewport != viewport_)
	{		
		glViewport(x, y, width, height);
		viewport_ = viewport;
	}
}

void accelerator::scissor(int x, int y, int width, int height)
{
	std::array<GLint, 4> scissor = {{x, y, width, height}};
	if(scissor != scissor_)
	{		
		enable(GL_SCISSOR_TEST);
		glScissor(x, y, width, height);
		scissor_ = scissor;
	}
}

void accelerator::stipple_pattern(const std::array<GLubyte, 32*32>& pattern)
{
	if(pattern_ != pattern)
	{		
		enable(GL_POLYGON_STIPPLE);

		std::array<GLubyte, 32*32> nopattern;
		nopattern.assign(0xFF);

		if(pattern == nopattern)
			disable(GL_POLYGON_STIPPLE);
		else
			glPolygonStipple(pattern.data());

		pattern_ = pattern;
	}
}

void accelerator::attach(device_buffer& texture)
{	
	if(attached_texture_ != texture.id())
	{
		if(attached_fbo_ != fbo_)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
			attached_fbo_ = fbo_;
		}

		GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, texture.id(), 0));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0));
		attached_texture_ = texture.id();
	}
}

void accelerator::clear(device_buffer& texture)
{	
	attach(texture);
	GL(glClear(GL_COLOR_BUFFER_BIT));
}

void accelerator::use(shader& shader)
{
	if(active_shader_ != shader.id())
	{		
		GL(glUseProgramObjectARB(shader.id()));	
		active_shader_ = shader.id();
	}
}

void accelerator::blend_func(int c1, int c2, int a1, int a2)
{
	std::array<int, 4> func = {c1, c2, a1, a2};

	if(blend_func_ != func)
	{
		enable(GL_BLEND);
		blend_func_ = func;
		GL(glBlendFuncSeparate(c1, c2, a1, a2));
	}
}

void accelerator::blend_func(int c1, int c2)
{
	blend_func(c1, c2, c1, c2);
}
	
boost::unique_future<safe_ptr<device_buffer>> accelerator::copy_async(safe_ptr<host_buffer>&& source, int width, int height, int stride)
{
	return executor_.begin_invoke([=]() -> safe_ptr<device_buffer>
	{
		auto result = create_device_buffer(width, height, stride);
		result->copy_from(source);
		return result;
	}, high_priority);
}

}}}

