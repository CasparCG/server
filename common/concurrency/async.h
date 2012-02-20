#pragma once

#include "../enum_class.h"

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility/declval.hpp>

#include <functional>
#include <memory>
#include <tuple>

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
struct invoke_callback
{	
	template<typename F>
	void operator()(boost::detail::future_object<R>& p, F& f)
	{
        p.mark_finished_with_result_internal(f());
	}
};

template<>
struct invoke_callback<void>
{	
	template<typename F>
	void operator()(boost::detail::future_object<void>& p, F& f)
	{
		f();
        p.mark_finished_with_result_internal();
	}
};

}
	
template<typename F>
auto async(launch lp, F&& f) -> boost::unique_future<decltype(f())>
{		
	typedef decltype(f()) result_type;
	
	if(lp == launch::deferred)
	{			
		// HACK: THIS IS A MAYOR HACK!

		typedef boost::detail::future_object<result_type> future_object_t;

		struct future_object_ex_t : public future_object_t
		{
			bool done;

			future_object_ex_t()
				: done(false)
			{
			}
		};
			
		auto future_object_ex	= boost::make_shared<future_object_ex_t>();
		bool& done				= future_object_ex->done;
		auto future_object		= boost::static_pointer_cast<future_object_t>(std::move(future_object_ex));
		auto future_object_raw	= future_object.get();

		int dummy = 0;
		future_object->set_wait_callback(std::function<void(int)>([f, future_object_raw, &done](int) mutable
		{								
			boost::lock_guard<boost::mutex> lock2(future_object_raw->mutex);
			
			if(done)
				return;

			detail::invoke_callback<result_type>()(*future_object_raw, f);
			
			done = true;
		}), &dummy);
		
		boost::unique_future<result_type> future;
		reinterpret_cast<boost::shared_ptr<future_object_t>&>(future) = std::move(future_object); // Get around the "private" encapsulation.
		return std::move(future);
	}
	else
	{
		typedef boost::packaged_task<decltype(f())> task_t;

		auto task	= task_t(std::forward<F>(f));	
		auto future = task.get_future();
		
		boost::thread(std::move(task)).detach();
	
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