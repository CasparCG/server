#pragma once

#include <chrono>
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

template <typename T>
struct _fold<std::shared_future<T>>
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
    return _fold<typename std::remove_reference<F>::type>::get(std::forward<F>(f));
}

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