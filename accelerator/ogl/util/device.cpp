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

#include "device.h"

#include "buffer.h"
#include "texture.h"
#include "shader.h"

#include <common/assert.h>
#include <common/except.h>
#include <common/concurrency/async.h>
#include <common/memory/array.h>
#include <common/gl/gl_check.h>
#include <common/os/windows/windows.h>


#include <boost/foreach.hpp>

#include <gl/glew.h>

#include <SFML/Window/Context.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>

#include <boost/utility/declval.hpp>

#include <array>
#include <unordered_map>

tbb::atomic<int> g_count = tbb::atomic<int>();

namespace caspar { namespace accelerator { namespace ogl {
		
struct device::impl : public std::enable_shared_from_this<impl>
{	
	static_assert(std::is_same<decltype(boost::declval<device>().impl_), spl::shared_ptr<impl>>::value, "impl_ must be shared_ptr");

	tbb::concurrent_hash_map<buffer*, std::shared_ptr<texture>> texture_mapping_;

	std::unique_ptr<sf::Context> device_;
	std::unique_ptr<sf::Context> host_alloc_device_;
	
	std::array<tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<std::shared_ptr<texture>>>, 4>	device_pools_;
	std::array<tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<std::shared_ptr<buffer>>>, 2>	host_pools_;
	
	GLuint fbo_;

	executor& render_executor_;
	executor  alloc_executor_;
				
	impl(executor& executor) 
		: render_executor_(executor)
		, alloc_executor_(L"OpenGL allocation context.")
	{
		if(g_count++ > 1)
			CASPAR_LOG(warning) << L"Multiple OGL devices.";

		CASPAR_LOG(info) << L"Initializing OpenGL Device.";
		
		auto ctx1 = render_executor_.invoke([=]() -> HGLRC 
		{
			device_.reset(new sf::Context());
			device_->SetActive(true);		
						
			if (glewInit() != GLEW_OK)
				BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
		
			if(!GLEW_VERSION_3_0)
				BOOST_THROW_EXCEPTION(not_supported() << msg_info("Your graphics card does not meet the minimum hardware requirements since it does not support OpenGL 3.0 or higher."));
	
			glGenFramebuffers(1, &fbo_);				
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
			
			auto ctx1 = wglGetCurrentContext();
			
			device_->SetActive(false);

			return ctx1;
		});

		alloc_executor_.invoke([=]
		{
			host_alloc_device_.reset(new sf::Context());
			host_alloc_device_->SetActive(true);	
			auto ctx2 = wglGetCurrentContext();

			if(!wglShareLists(ctx1, ctx2))
				BOOST_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to share OpenGL devices."));
		});

		render_executor_.invoke([=]
		{		
			device_->SetActive(true);
		});
		
		CASPAR_LOG(info) << L"Successfully initialized OpenGL " << version();
	}

	~impl()
	{
		alloc_executor_.invoke([=]
		{
			host_alloc_device_.reset();
			BOOST_FOREACH(auto& pool, host_pools_)
				pool.clear();
		});

		render_executor_.invoke([=]
		{
			BOOST_FOREACH(auto& pool, device_pools_)
				pool.clear();
			glDeleteFramebuffers(1, &fbo_);

			device_.reset();
		});
	}
		
	std::wstring version()
	{	
		try
		{
			return alloc_executor_.invoke([]
			{
				return u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VERSION)))) + L" " + u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VENDOR))));
			});	
		}
		catch(...)
		{
			return L"Not found";;
		}
	}
							
	spl::shared_ptr<texture> create_texture(int width, int height, int stride)
	{
		CASPAR_VERIFY(stride > 0 && stride < 5);
		CASPAR_VERIFY(width > 0 && height > 0);
		
		auto pool = &device_pools_[stride-1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
		
		std::shared_ptr<texture> buffer;
		if(!pool->try_pop(buffer))		
			buffer = spl::make_shared<texture>(width, height, stride);
	
		return spl::shared_ptr<texture>(buffer.get(), [buffer, pool](texture*) mutable
		{		
			pool->push(buffer);	
		});
	}
		
	spl::shared_ptr<buffer> create_buffer(int size, buffer::usage usage)
	{
		CASPAR_VERIFY(size > 0);
		
		auto pool = &host_pools_[usage.value()][size];
		
		std::shared_ptr<buffer> buf;
		if(!pool->try_pop(buf))	
		{
			buf = alloc_executor_.invoke([&]
			{
				return spl::make_shared<buffer>(size, usage);
			});
		}
				
		auto ptr = buf->data();
		auto self = shared_from_this(); // buffers can leave the device context, take a hold on life-time.

		auto on_release = [self, buf, ptr, usage, pool]() mutable
		{		
			if(usage == buffer::usage::write_only)					
				buf->map();					
			else
				buf->unmap();

			self->texture_mapping_.erase(buf.get());

			pool->push(buf);
		};
		
		return spl::shared_ptr<buffer>(buf.get(), [=](buffer*) mutable
		{
			self->alloc_executor_.begin_invoke(on_release);	
		});
	}

	core::mutable_array create_array(int size)
	{		
		auto buf = create_buffer(size, buffer::usage::write_only);
		return core::mutable_array(buf->data(), buf->size(), buf);
	}

	boost::unique_future<spl::shared_ptr<texture>> copy_async(const core::mutable_array& source, int width, int height, int stride)
	{
		auto buf = boost::any_cast<spl::shared_ptr<buffer>>(source.storage());
				
		return render_executor_.begin_invoke([=]() -> spl::shared_ptr<texture>
		{
			tbb::concurrent_hash_map<buffer*, std::shared_ptr<texture>>::const_accessor a;
			if(texture_mapping_.find(a, buf.get()))
				return spl::make_shared_ptr(a->second);

			auto texture = create_texture(width, height, stride);
			texture->copy_from(*buf);	

			texture_mapping_.insert(std::make_pair(buf.get(), texture));

			return texture;

		}, task_priority::high_priority);
	}

	boost::unique_future<core::const_array> copy_async(const spl::shared_ptr<texture>& source)
	{
		return fold(render_executor_.begin_invoke([=]() -> boost::shared_future<core::const_array>
		{
			auto buffer = create_buffer(source->size(), buffer::usage::read_only); 
			source->copy_to(*buffer);	

			return make_shared(async(launch_policy::deferred, [=]() mutable -> core::const_array
			{
				const auto& buf = buffer.get();
				if(!buf->data())
					alloc_executor_.invoke(std::bind(&buffer::map, std::ref(buf))); // Defer blocking "map" call until data is needed.

				return core::const_array(buf->data(), buf->size(), buffer);
			}));
		}, task_priority::high_priority));
	}
};

device::device() 
	: executor_(L"OpenGL Rendering Context.")
	, impl_(new impl(executor_))
{
}
device::~device(){}	
spl::shared_ptr<texture>							device::create_texture(int width, int height, int stride){return impl_->create_texture(width, height, stride);}
core::mutable_array									device::create_array(int size){return impl_->create_array(size);}
boost::unique_future<spl::shared_ptr<texture>>		device::copy_async(const core::mutable_array& source, int width, int height, int stride){return impl_->copy_async(source, width, height, stride);}
boost::unique_future<core::const_array>				device::copy_async(const spl::shared_ptr<texture>& source){return impl_->copy_async(source);}
std::wstring										device::version(){return impl_->version();}


}}}


