#include "../../stdafx.h"

#include "ogl_device.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <boost/foreach.hpp>

namespace caspar { namespace core {

ogl_device::ogl_device()
{
	executor_.start();
	invoke([=]
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
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
	});
}
				
safe_ptr<device_buffer> ogl_device::create_device_buffer(size_t width, size_t height, size_t stride)
{
	auto pool = &device_pools_[stride][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];
	std::shared_ptr<device_buffer> buffer;
	if(!pool->try_pop(buffer))		
	{
		executor_.invoke([&]
		{
			buffer = std::make_shared<device_buffer>(width, height, stride);
		});	
	}
		
	return safe_ptr<device_buffer>(buffer.get(), [=](device_buffer*){pool->push(buffer);});
}
	
safe_ptr<host_buffer> ogl_device::create_host_buffer(size_t size, host_buffer::usage_t usage)
{
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
		});	
	}

	return safe_ptr<host_buffer>(buffer.get(), [=](host_buffer*)
	{
		executor_.begin_invoke([=]
		{
			if(usage == host_buffer::write_only)
				buffer->map();
			else
				buffer->unmap();
			pool->push(buffer);
		});
	});
}

}}