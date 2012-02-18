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
typedef enum_class<launch_policy_def> launch;

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
auto async(launch lp, F&& f) -> boost::unique_future<decltype(f())>
{		
	typedef decltype(f()) result_type;
	
	typedef boost::promise<result_type> promise_t;

	// WORKAROUND: Use heap storage since lambdas don't support move semantics and do a lot of unnecessary copies.
	auto promise = new promise_t();
	auto func	 = new F(std::forward<F>(f));
	auto future  = promise->get_future();

	if(lp == launch::deferred)
	{	
		promise->set_wait_callback(std::function<void(promise_t&)>([=](promise_t&) mutable
		{
			std::unique_ptr<promise_t> pointer_guard(promise);
			std::unique_ptr<F>		   func_guard(func);
			detail::invoke_function<result_type>()(*promise, *func);
		}));

		return std::move(future);
	}
	else
	{
		boost::thread([=]
		{
			std::unique_ptr<promise_t> pointer_guard(promise);
			std::unique_ptr<F>		   func_guard(func);

			try
			{				
				detail::invoke_function<result_type>()(*promise, *func);
			}
			catch(...)
			{
				promise->set_exception(boost::current_exception());
			}
		}).detach();

		return std::move(future);
	}
}
	
template<typename F>
auto async(F&& f) -> boost::unique_future<decltype(f())>
{	
	return async(launch::async, std::forward<F>(f));
}

template<typename T>
auto make_shared(boost::unique_future<T>&& f) -> boost::shared_future<T>
{	
	return boost::shared_future<T>(std::move(f));
}

template<typename T>
auto flatten(boost::unique_future<T>&& f) -> boost::unique_future<decltype(f.get().get())>
{
	auto shared_f = make_shared(std::move(f));
	return async(launch::deferred, [=]() mutable
	{
		return shared_f.get().get();
	});
}

}