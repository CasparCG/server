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

#include "ogl_device.h"

#include "shader.h"
#include "fence.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/scope_guard.h>

#include <common/assert.h>
#include <boost/foreach.hpp>

#include <gl/glew.h>

namespace caspar { namespace core {

ogl_device::ogl_device() 
	: executor_(L"ogl_device")
{
	state_stack_.push(state());

	CASPAR_LOG(info) << L"Initializing OpenGL Device.";
		
	invoke([=]
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
		
		if (glewInit() != GLEW_OK)
			BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
						
		CASPAR_LOG(info) << L"OpenGL " << version();

		if(!GLEW_VERSION_3_0)
			CASPAR_LOG(warning) << "Missing OpenGL 3.0 support.";
	
		CASPAR_LOG(info) << L"Successfully initialized GLEW.";

		glGenFramebuffers(1, &fbo_);	

		CASPAR_LOG(debug) << "Created framebuffer.";

		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

		CASPAR_LOG(info) << L"Successfully initialized OpenGL Device.";

		enable(GL_TEXTURE_2D);
	});
}

ogl_device::~ogl_device()
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

safe_ptr<device_buffer> ogl_device::allocate_device_buffer(int width, int height, int stride)
{
	std::shared_ptr<device_buffer> buffer;
	try
	{
		buffer.reset(new device_buffer(width, height, stride));
	}
	catch(...)
	{
		try
		{
			yield();
			gc().wait();
					
			// Try again
			buffer.reset(new device_buffer(width, height, stride));
		}
		catch(...)
		{
			CASPAR_LOG(error) << L"ogl: create_device_buffer failed!";
			throw;
		}
	}
	return make_safe_ptr(buffer);
}
				
safe_ptr<device_buffer> ogl_device::create_device_buffer(int width, int height, int stride)
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

safe_ptr<host_buffer> ogl_device::allocate_host_buffer(int size, host_buffer::usage_t usage)
{
	std::shared_ptr<host_buffer> buffer;

	try
	{
		buffer.reset(new host_buffer(size, usage));
		if(usage == host_buffer::write_only)
			buffer->map();
		else
			buffer->unmap();			
	}
	catch(...)
	{
		try
		{
			yield();
			gc().wait();

			// Try again
			buffer.reset(new host_buffer(size, usage));
			if(usage == host_buffer::write_only)
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
	
safe_ptr<host_buffer> ogl_device::create_host_buffer(int size, host_buffer::usage_t usage)
{
	CASPAR_VERIFY(usage == host_buffer::write_only || usage == host_buffer::read_only);
	CASPAR_VERIFY(size > 0);
	auto& pool = host_pools_[usage][size];
	std::shared_ptr<host_buffer> buffer;
	if(!pool->items.try_pop(buffer))	
		buffer = executor_.invoke([=]{return allocate_host_buffer(size, usage);}, high_priority);	
	
	//++pool->usage_count;

	auto self = shared_from_this();
	return safe_ptr<host_buffer>(buffer.get(), [=](host_buffer*) mutable
	{
		self->executor_.begin_invoke([=]() mutable
		{		
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();

			pool->items.push(buffer);
		}, high_priority);	
	});
}

safe_ptr<ogl_device> ogl_device::create()
{
	return safe_ptr<ogl_device>(new ogl_device());
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

void ogl_device::flush()
{
	GL(glFlush());	
		
	//try
	//{
	//	BOOST_FOREACH(auto& pools, device_pools_)
	//	{
	//		BOOST_FOREACH(auto& pool, pools)
	//			flush_pool(*pool.second);
	//	}
	//	BOOST_FOREACH(auto& pools, host_pools_)
	//	{
	//		BOOST_FOREACH(auto& pool, pools)
	//			flush_pool(*pool.second);
	//	}
	//}
	//catch(...)
	//{
	//	CASPAR_LOG_CURRENT_EXCEPTION();
	//}
}

void ogl_device::yield()
{
	push_state();
	auto restore_state = make_scope_guard([&]
	{
		pop_state();
	});
	
	executor_.yield();
}

boost::unique_future<void> ogl_device::gc()
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

std::wstring ogl_device::version()
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

void ogl_device::push_state()
{
	state_stack_.push(state_);
}

void ogl_device::pop_state()
{
	if(state_stack_.size() == 1)
		BOOST_THROW_EXCEPTION(invalid_operation());

	state_stack_.pop();
	auto new_state = state_stack_.top();
	viewport(new_state.viewport[0], new_state.viewport[1], new_state.viewport[2], new_state.viewport[3]);
	scissor(new_state.scissor[0], new_state.scissor[1], new_state.scissor[2], new_state.scissor[3]);
	stipple_pattern(new_state.pattern.data());
	blend_func(new_state.blend_func[0], new_state.blend_func[1], new_state.blend_func[2], new_state.blend_func[3]);
	attach(new_state.attached_texture);
	use(new_state.active_shader);
	for(int n = 0; n < 16; ++n)
		bind(new_state.binded_textures[n], n);
}

void ogl_device::enable(GLenum cap)
{
	auto& val = caps_[cap];
	if(!val)
	{
		glEnable(cap);
		val = true;
	}
}

void ogl_device::disable(GLenum cap)
{
	auto& val = caps_[cap];
	if(val)
	{
		glDisable(cap);
		val = false;
	}
}

void ogl_device::viewport(int x, int y, int width, int height)
{
	std::array<int, 4> viewport = {{x, y, width, height}};
	if(viewport != state_.viewport)
	{		
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		state_.viewport = viewport;
	}
}

void ogl_device::scissor(int x, int y, int width, int height)
{
	std::array<int, 4> scissor = {{x, y, width, height}};
	if(scissor != state_.scissor)
	{		
		state def_state_;
		if(scissor == def_state_.scissor)
		{
			disable(GL_SCISSOR_TEST);
		}
		else
		{
			enable(GL_SCISSOR_TEST);
			glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
			state_.scissor = scissor;
		}
	}
}

void ogl_device::stipple_pattern(const GLubyte* pattern)
{
	std::array<GLubyte, 32*32> pattern2;
	memcpy(pattern2.data(), pattern, 32*32);

	if(pattern2 != state_.pattern)
	{
		state def_state_;
		if(pattern2 == def_state_.pattern)
			disable(GL_POLYGON_STIPPLE);
		else
		{
			enable(GL_POLYGON_STIPPLE);
			glPolygonStipple(pattern2.data());
			state_.pattern = pattern2;
		}
	}
}

void ogl_device::attach(GLint id)
{	
	if(id != state_.attached_texture)
	{
		GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, id, 0));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0));

		state_.attached_texture = id;
	}
}

void ogl_device::attach(const device_buffer& texture)
{	
	attach(texture.id());
}

void ogl_device::bind(GLint id, int index)
{
	if(id != state_.binded_textures[index])
	{		
		GL(glActiveTexture(GL_TEXTURE0+index));
		glBindTexture(GL_TEXTURE_2D, id);
	}
}

void ogl_device::bind(const device_buffer& texture, int index)
{
	//while(true)
	//{
	//	if(!texture.ready())		
	//		yield();
	//	else
	//		break;
	//}

	bind(texture.id(), index);
}

void ogl_device::clear(device_buffer& texture)
{	
	auto prev = state_.attached_texture;
	attach(texture);
	auto restore_attachement = make_scope_guard([&]
	{
		attach(prev);
	});

	GL(glClear(GL_COLOR_BUFFER_BIT));
}

void ogl_device::use(GLint id)
{
	if(id != state_.active_shader)
	{		
		GL(glUseProgramObjectARB(id));	
		state_.active_shader = id;
	}
}

void ogl_device::use(const shader& shader)
{
	use(shader.id());
}

void ogl_device::blend_func(int c1, int c2, int a1, int a2)
{
	std::array<int, 4> func = {c1, c2, a1, a2};

	if(state_.blend_func != func)
	{
		state def_state_;
		if(func == def_state_.blend_func)
			disable(GL_BLEND);
		else
		{
			enable(GL_BLEND);
			GL(glBlendFuncSeparate(c1, c2, a1, a2));
			state_.blend_func = func;
		}
	}
}

void ogl_device::blend_func(int c1, int c2)
{
	blend_func(c1, c2, c1, c2);
}

boost::unique_future<safe_ptr<host_buffer>> ogl_device::transfer(const safe_ptr<device_buffer>& source)
{		
	return begin_invoke([=]() -> safe_ptr<host_buffer>
	{
		push_state();
		auto restore_state = make_scope_guard([&]
		{
			pop_state();
		});

		auto dest = create_host_buffer(source->width()*source->height()*source->stride(), host_buffer::read_only);

		attach(*source);
	
		dest->bind();
		GL(glReadPixels(0, 0, source->width(), source->height(), format(source->stride()), GL_UNSIGNED_BYTE, NULL));
		dest->unbind();
	
		auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		flush();
	
		GLsizei length = 0;
		int values[] = {0};
	
		while(true)
		{
			GL(glGetSynciv(sync, GL_SYNC_STATUS, 1, &length, values));

			if(values[0] != GL_SIGNALED)		
				yield();
			else
				break;
		}

		dest->map();

		return dest;
	});
}

boost::unique_future<safe_ptr<device_buffer>> ogl_device::transfer(const safe_ptr<host_buffer>& source, int width, int height, int stride)
{		
	return begin_invoke([=]() -> safe_ptr<device_buffer>
	{
		push_state();
		auto restore_state = make_scope_guard([&]
		{
			pop_state();
		});
		
		auto dest = create_device_buffer(width, height, stride);

		source->unmap();
		source->bind();
			
		const_cast<ogl_device*>(this)->bind(*dest, 0); // WORKAROUND: const-cast needed due to compiler bug?
		GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dest->width(), dest->height(), format(dest->stride()), GL_UNSIGNED_BYTE, NULL));
			
		source->unbind();

		return dest;
	});
}

}}

