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
// TODO: Smart GC

#include "../../stdafx.h"

#include "ogl_device.h"

#include "shader.h"
#include "fence.h"

#include <common/exception/exceptions.h>
#include <common/utility/assert.h>
#include <common/gl/gl_check.h>

#include <boost/foreach.hpp>

#include <gl/glew.h>

namespace caspar { namespace core {

ogl_device::ogl_device() 
	: executor_(new executor(L"ogl_device"))
	, pattern_(nullptr)
	, attached_texture_(0)
	, active_shader_(0)
{
	graph_->set_color("fence", diagnostics::color(1.0f, 0.0f, 0.0f));
	graph_->set_text("gpu");
	diagnostics::register_graph(graph_);

	std::fill(binded_textures_.begin(), binded_textures_.end(), 0);
	std::fill(viewport_.begin(), viewport_.end(), 0);
	std::fill(scissor_.begin(), scissor_.end(), 0);
	std::fill(blend_func_.begin(), blend_func_.end(), 0);
	
	invoke([=]
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
		
		if (glewInit() != GLEW_OK)
			BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
				
		if(!GLEW_VERSION_3_2)
			CASPAR_LOG(warning) << "Missing OpenGL 3.2 support.";
	
		GL(glGenFramebuffers(1, &fbo_));		
		GL(glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo_));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT));
        GL(glDisable(GL_MULTISAMPLE_ARB));
	});

	CASPAR_LOG(info) << L"Successfully Initialized GPU-Device";
}

ogl_device::~ogl_device()
{
	invoke([=]
	{
		BOOST_FOREACH(auto& pool, device_pools_)
			pool.clear();
		BOOST_FOREACH(auto& pool, host_pools_)
			pool.clear();
		glDeleteFramebuffersEXT(1, &fbo_);
	});
	
	executor_->stop();
	executor_->join();

	CASPAR_LOG(info) << L"Successfully Uninitialized GPU-Device";
}

safe_ptr<device_buffer> ogl_device::allocate_device_buffer(size_t width, size_t height, size_t stride)
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
			CASPAR_LOG(error) << L"[ogl_device] create_device_buffer failed!";
			throw;
		}
	}
	return make_safe_ptr(buffer);
}
				
safe_ptr<device_buffer> ogl_device::create_device_buffer(size_t width, size_t height, size_t stride)
{
	CASPAR_VERIFY(stride > 0 && stride < 5, invalid_argument());
	CASPAR_VERIFY(width > 0 && height > 0, invalid_argument());
	auto& pool = device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
	std::shared_ptr<device_buffer> buffer;
	if(!pool->items.try_pop(buffer))		
		buffer = executor_->invoke([&]{return allocate_device_buffer(width, height, stride);}, high_priority);			
	
	//++pool->usage_count;

	auto self = safe_from_this();
	return safe_ptr<device_buffer>(buffer.get(), [self, pool, buffer](device_buffer*) mutable
	{		
		pool->items.push(buffer);	
	});
}

safe_ptr<host_buffer> ogl_device::allocate_host_buffer(size_t size, host_buffer::usage_t usage)
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
			CASPAR_LOG(error) << L"[ogl_device] create_host_buffer failed!";
			throw;		
		}
	}

	return make_safe_ptr(buffer);
}
	
safe_ptr<host_buffer> ogl_device::create_host_buffer(size_t size, host_buffer::usage_t usage)
{
	CASPAR_VERIFY(usage == host_buffer::write_only || usage == host_buffer::read_only, invalid_argument());
	CASPAR_VERIFY(size > 0, invalid_argument());
	auto& pool = host_pools_[usage][size];
	std::shared_ptr<host_buffer> buffer;
	if(!pool->items.try_pop(buffer))	
		buffer = executor_->invoke([=]{return allocate_host_buffer(size, usage);}, high_priority);	
	
	//++pool->usage_count;
	
	auto self = safe_from_this();
	return safe_ptr<host_buffer>(buffer.get(), [self, pool, buffer, usage](host_buffer*)
	{
		self->begin_invoke([=]()
		{		
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();

			pool->items.push(buffer);
		}, high_priority);	
	});
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
	executor_->yield();
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

std::wstring ogl_device::get_version()
{	
	static std::wstring ver = L"Not found";
	try
	{
		ogl_device tmp;
		ver = widen(tmp.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));})
		+ " "	+ tmp.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));}));			
	}
	catch(...){}

	return ver;
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

void ogl_device::viewport(size_t x, size_t y, size_t width, size_t height)
{
	if(x != viewport_[0] || y != viewport_[1] || width != viewport_[2] || height != viewport_[3])
	{		
		glViewport(x, y, width, height);
		viewport_[0] = x;
		viewport_[1] = y;
		viewport_[2] = width;
		viewport_[3] = height;
	}
}

void ogl_device::scissor(size_t x, size_t y, size_t width, size_t height)
{
	if(x != scissor_[0] || y != scissor_[1] || width != scissor_[2] || height != scissor_[3])
	{		
		glScissor(x, y, width, height);
		scissor_[0] = x;
		scissor_[1] = y;
		scissor_[2] = width;
		scissor_[3] = height;
	}
}

void ogl_device::stipple_pattern(const GLubyte* pattern)
{
	if(pattern_ != pattern)
	{		
		glPolygonStipple(pattern);
		pattern_ = pattern;
	}
}

void ogl_device::attach(device_buffer& texture)
{	
	if(attached_texture_ != texture.id())
	{
		GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, texture.id(), 0));
		attached_texture_ = texture.id();
	}
}

void ogl_device::clear(device_buffer& texture)
{	
	attach(texture);
	GL(glClear(GL_COLOR_BUFFER_BIT));
}

void ogl_device::use(shader& shader)
{
	if(active_shader_ != shader.id())
	{		
		GL(glUseProgramObjectARB(shader.id()));	
		active_shader_ = shader.id();
	}
}

void ogl_device::blend_func(int c1, int c2, int a1, int a2)
{
	std::array<int, 4> func = {c1, c2, a1, a2};

	if(blend_func_ != func)
	{
		blend_func_ = func;
		GL(glBlendFuncSeparate(c1, c2, a1, a2));
	}
}

void ogl_device::blend_func(int c1, int c2)
{
	blend_func(c1, c2, c1, c2);
}

void ogl_device::wait(const fence& fence)
{
	const int MAX_DELAY = 40;

	int delay = 0;
	if(!invoke([&]{return fence.ready();}, high_priority))
	{
		while(!invoke([&]{return fence.ready();}, normal_priority) && delay < MAX_DELAY)
		{
			delay += 4;
			Sleep(4);
		}
	}
	graph_->update_value("fence", delay/static_cast<double>(MAX_DELAY));
		
	static tbb::atomic<size_t> count;
	static tbb::atomic<bool> warned;
		
	if(delay > 2 && ++count > 8)
	{
		if(!warned.fetch_and_store(true))
		{
			CASPAR_LOG(warning) << L"[fence] Performance warning. GPU was not ready during requested host read-back. Delayed by atleast: " << delay << L" ms. Further warnings are sent to diagnostics."
								<< L" You can ignore this warning if you do not notice any problems with output video. This warning is caused by insufficent support or performance of your graphics card for OpenGL based memory transfers. "
								<< L" Please try to update your graphics drivers or update your graphics card, see recommendations on (www.casparcg.com)."
								<< L" Further help is available at (www.casparcg.com/forum).";
		}
	}
}

}}

