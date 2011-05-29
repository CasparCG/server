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
#include "../../stdafx.h"

#include "ogl_device.h"

#include <common/utility/assert.h>
#include <common/gl/gl_check.h>

#include <boost/foreach.hpp>

namespace caspar { namespace core {

ogl_device::ogl_device() : executor_(L"ogl_device")
{
	invoke([=]
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
						
		if(!GLEE_VERSION_3_0)
			BOOST_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 3.0 support."));

		GL(glGenFramebuffers(1, &fbo_));		
		GL(glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo_));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT));
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
		glDeleteFramebuffersEXT(1, &fbo_);
	});
}
				
safe_ptr<device_buffer> ogl_device::create_device_buffer(size_t width, size_t height, size_t stride)
{
	CASPAR_VERIFY(stride > 0 && stride < 5);
	CASPAR_VERIFY(width > 0 && height > 0);
	auto pool = &device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
	std::shared_ptr<device_buffer> buffer;
	if(!pool->try_pop(buffer))		
	{
		executor_.invoke([&]
		{
			buffer = std::make_shared<device_buffer>(width, height, stride);

			if(glGetError() != GL_NO_ERROR)
				BOOST_THROW_EXCEPTION(std::bad_alloc());

		}, high_priority);	
		executor_.begin_invoke([=]
		{
			auto buffer = std::make_shared<device_buffer>(width, height, stride);
			pool->try_push(buffer);
		}, high_priority);	
	}
			
	return safe_ptr<device_buffer>(buffer.get(), [=](device_buffer*){pool->push(buffer);});
}
	
safe_ptr<host_buffer> ogl_device::create_host_buffer(size_t size, host_buffer::usage_t usage)
{
	CASPAR_VERIFY(usage == host_buffer::write_only || usage == host_buffer::read_only);
	CASPAR_VERIFY(size > 0);
	auto pool = &host_pools_[usage][size];
	std::shared_ptr<host_buffer> buffer;
	if(!pool->try_pop(buffer))
	{
		executor_.invoke([&]
		{
			buffer = std::make_shared<host_buffer>(size, usage);
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();

			if(glGetError() != GL_NO_ERROR)
				BOOST_THROW_EXCEPTION(std::bad_alloc());

		}, high_priority);	
		executor_.begin_invoke([=]
		{
			auto buffer = std::make_shared<host_buffer>(size, usage);
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();
			pool->try_push(buffer);
		}, high_priority);	
	}
	
	return safe_ptr<host_buffer>(buffer.get(), [=](host_buffer*)
	{
		executor_.begin_invoke([=]
		{
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();
			
			if(glGetError() == GL_NO_ERROR)
				pool->push(buffer);
		}, high_priority);
	});
}

void ogl_device::yield()
{
	executor_.yield();
}

std::wstring ogl_device::get_version()
{	
	static std::wstring ver;
	if(ver.empty())
	{
		ogl_device tmp;
		ver = widen(tmp.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));})
		+ " "	+ tmp.invoke([]{return std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));}));	
	}
	return ver;
}

}}