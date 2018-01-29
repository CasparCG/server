#pragma once

#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/spawn.hpp>

#include <future>

namespace caspar {

template<typename C, typename Func>
auto dispatch_async(C& context, Func&& func)
{
	typedef decltype(func())                  result_type;
	typedef std::packaged_task<result_type()> task_type;

    auto task = task_type(std::forward<Func>(func));
    auto future = task.get_future();
    boost::asio::dispatch(context, task_type(std::forward<Func>(func)));
    return future;
}

template<typename C, typename Func>
auto post_async(C& context, Func&& func)
{
	typedef decltype(func())                  result_type;
	typedef std::packaged_task<result_type()> task_type;

    auto task = task_type(std::forward<Func>(func));
    auto future = task.get_future();
    boost::asio::post(context, task_type(std::forward<Func>(func)));
    return future;
}

template<typename C, typename Func>
auto spawn_async(C& context, Func&& func)
{
	typedef decltype(func(std::declval<yield_context>()))  result_type;
	typedef std::packaged_task<result_type(yield_context)> task_type;

    auto task = task_type(std::forward<Func>(func));
    auto future = task.get_future();
    boost::asio::spawn(context, std::move(task));
    return future;
}

}