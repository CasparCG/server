#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <future>

namespace caspar {

template <typename T>
struct _fold
{
     template <typename F>
     static auto get(F&& f)
     {
        return std::move(f);
     }
};

template <typename T>
struct _fold<std::future<T>>
{
     template <typename F>
     static auto get(F&& f)
     {
        return std::async(std::launch::deferred, [](auto f)
        {
            return _fold<decltype(f.get().get())>::get(f.get()).get();
        }, std::forward<F>(f));
     }
};

template <typename F>
auto fold(F&& f)
{
    return _fold<decltype(f.get())>::get(std::move(f));
}

template<typename F>
bool is_ready(const F& future)
{
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template<class R>
std::future<R> make_ready_future(R&& value)
{
	std::promise<R> p;

	p.set_value(std::forward<R>(value));

	return p.get_future();
}

static std::future<void> make_ready_future()
{
	std::promise<void> p;

	p.set_value();

	return p.get_future();
}

}