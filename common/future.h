#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <future>

namespace caspar {

template<typename T>
auto flatten(std::future<T>&& f) -> std::future<typename std::decay<decltype(f.get().get())>::type>
{
	auto shared_f = f.share();
	return std::async(std::launch::deferred, [=]() mutable -> typename std::decay<decltype(f.get().get())>::type
	{
		return shared_f.get().get();
	});
}

template<typename F>
bool is_ready(const F& future)
{
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

/**
 * Wrap a value in a future with an already known result.
 * <p>
 * Useful when the result of an operation is already known at the time of
 * calling.
 *
 * @param value The r-value to wrap.
 *
 * @return The future with the result set.
 */
template<class R>
std::future<R> make_ready_future(R&& value)
{
	std::promise<R> p;

	p.set_value(value);

	return p.get_future();
}

static std::future<void> make_ready_future()
{
	std::promise<void> p;

	p.set_value();

	return p.get_future();
}

}