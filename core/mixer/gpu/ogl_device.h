#pragma once

#include "host_buffer.h"
#include "device_buffer.h"

#include <common/concurrency/executor.h>
#include <common/memory/safe_ptr.h>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread/future.hpp>

#include <array>

namespace caspar { namespace core {

class ogl_device
{	
public:	
	ogl_device();

	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
	{			
		return executor_.begin_invoke(std::forward<Func>(func));
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		return executor_.invoke(std::forward<Func>(func));
	}
		
	safe_ptr<device_buffer> create_device_buffer(size_t width, size_t height, size_t stride);
	safe_ptr<host_buffer> create_host_buffer(size_t size, host_buffer::usage_t usage);

private:
	executor executor_;
	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<host_buffer>>>, 2> host_pools_;
};

}}