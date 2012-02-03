#pragma once

#include <functional>
#include <memory>

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>

#include <boost/utility/declval.hpp>

#include "../enum_class.h"

namespace caspar {

struct launch_policy_def
{
	enum type
	{
		async,
		deferred
	};
};
typedef enum_class<launch_policy_def> launch_policy;

namespace detail {

template<typename R>
struct invoke_function
{	
	template<typename F>
	void operator()(boost::promise<R>& p, F& f)
	{
		p.set_value(f());
	}
};

template<>
struct invoke_function<void>
{	
	template<typename F>
	void operator()(boost::promise<void>& p, F& f)
	{
		f();
		p.set_value();
	}
};

}
	
template<typename F>
auto async(launch_policy lp, F&& f) -> boost::unique_future<decltype(f())>
{		
	typedef decltype(f()) result_type;

	if(lp == launch_policy::deferred)
	{
		typedef boost::promise<result_type> promise_t;

		auto promise = new promise_t();
		auto future  = promise->get_future();
	
		promise->set_wait_callback(std::function<void(promise_t&)>([=](promise_t&) mutable
		{
			std::unique_ptr<promise_t> pointer_guard(promise);
			detail::invoke_function<result_type>()(*promise, f);
		}));

		return std::move(future);
	}
	else
	{
		typedef boost::packaged_task<result_type> packaged_task_t;

		auto task   = packaged_task_t(f);    
		auto future = task.get_future();

		boost::thread(std::move(task)).detach();

		return std::move(future);
	}
}
	
template<typename F>
auto async(F&& f) -> boost::unique_future<decltype(f())>
{	
	return async(launch_policy::async, std::forward<F>(f));
}

}