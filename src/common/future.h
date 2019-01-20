#pragma once

#include <chrono>
#include <future>
#include <type_traits>

namespace caspar {

template <typename F>
auto flatten(F&& f)
{
    return std::async(std::launch::deferred, [f = std::forward<F>(f)]() mutable { return f.get().get(); });
}

template <typename F>
bool is_ready(const F& future)
{
    return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template <class T>
auto make_ready_future(T&& value)
{
    std::promise<typename std::decay<T>::type> p;
    p.set_value(std::forward<T>(value));
    return p.get_future();
}

static std::future<void> make_ready_future()
{
    std::promise<void> p;
    p.set_value();
    return p.get_future();
}

} // namespace caspar