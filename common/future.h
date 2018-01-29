#pragma once

#include <chrono>
#include <future>

namespace caspar {

template<typename F>
bool is_ready(const F& future)
{
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template<class T>
std::future<T> make_ready_future(T&& value)
{
	std::promise<T> p;
	p.set_value(std::forward<T>(value));
	return p.get_future();
}

static std::future<void> make_ready_future()
{
	std::promise<void> p;
	p.set_value();
	return p.get_future();
}

}