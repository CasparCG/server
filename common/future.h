#pragma once

#include <boost/function.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <future>

namespace caspar {

template <typename F>
auto flatten(F&& f)
{
    return std::async(std::launch::deferred, [](F&& f) { return f.get().get(); }, std::forward<F>(f));
}

template <typename F>
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
template <class R>
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

} // namespace caspar