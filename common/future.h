#pragma once

#include "enum_class.h"

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <functional>

namespace caspar {
	
struct launch_policy_def
{
	enum type
	{
		async = 1,
		deferred = 2
	};
};
typedef caspar::enum_class<launch_policy_def> launch;

namespace detail {

template<typename R, typename F>
struct callback_object: public boost::detail::future_object<R>
{	
	F f;
	bool done;

	template<typename F2>
	callback_object(F2&& f)
		: f(std::forward<F2>(f))
		, done(false)
	{
	}
		
	void operator()()
	{		
		boost::lock_guard<boost::mutex> lock2(mutex);

		if(done)
			return;

        try
        {
			this->mark_finished_with_result_internal(f());
        }
        catch(...)
        {
			this->mark_exceptional_finish_internal(boost::current_exception());
        }

		done = true;
	}
};

template<typename F>
struct callback_object<void, F> : public boost::detail::future_object<void>
{	
	F f;
	bool done;

	template<typename F2>
	callback_object(F2&& f)
		: f(std::forward<F2>(f))
		, done(false)
	{
	}

	void operator()()
	{
		boost::lock_guard<boost::mutex> lock2(mutex);
		
		if(done)
			return;

        try
        {
			f();
			this->mark_finished_with_result_internal();
		}
        catch(...)
        {
			this->mark_exceptional_finish_internal(boost::current_exception());
        }

		done = true;
	}
};

}
	
template<typename F>
auto async(launch policy, F&& f) -> boost::unique_future<decltype(f())>
{		
	typedef decltype(f()) result_type;
	
	if((policy & launch::async) != 0)
	{
		typedef boost::packaged_task<result_type> task_t;

		auto task	= task_t(std::forward<F>(f));	
		auto future = task.get_future();
		
		boost::thread(std::move(task)).detach();
	
		return std::move(future);
	}
	else if((policy & launch::deferred) != 0)
	{		
		// HACK: THIS IS A MAYOR HACK!

		typedef boost::detail::future_object<result_type> future_object_t;
					
		auto callback_object	 = boost::make_shared<detail::callback_object<result_type, F>>(std::forward<F>(f));
		auto callback_object_raw = callback_object.get();
		auto future_object		 = boost::static_pointer_cast<future_object_t>(std::move(callback_object));

		future_object->set_wait_callback(std::mem_fn(&detail::callback_object<result_type, F>::operator()), callback_object_raw);
		
		boost::unique_future<result_type> future;

		static_assert(sizeof(future) == sizeof(future_object), "");

		reinterpret_cast<boost::shared_ptr<future_object_t>&>(future) = std::move(future_object); // Get around the "private" encapsulation.
		return std::move(future);
	}
	else
		throw std::invalid_argument("policy");
}
	
template<typename F>
auto async(F&& f) -> boost::unique_future<decltype(f())>
{	
	return async(launch::async | launch::deferred, std::forward<F>(f));
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