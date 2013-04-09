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
	
template<typename R>
struct future_object_helper
{	
	template<typename T, typename F>
	static void nonlocking_invoke(T& future_object, F& f)
	{				
        try
        {
			future_object.mark_finished_with_result_internal(f());
        }
        catch(...)
        {
			future_object.mark_exceptional_finish_internal(boost::current_exception());
        }
	}

	template<typename T, typename F>
	static void locking_invoke(T& future_object, F& f)
	{				
        try
        {
			future_object.mark_finished_with_result(f());
        }
        catch(...)
        {
			future_object.mark_exceptional_finish();
        }
	}
};

template<>
struct future_object_helper<void>
{	
	template<typename T, typename F>
	static void nonlocking_invoke(T& future_object, F& f)
	{				
        try
        {
			f();
			future_object.mark_finished_with_result_internal();
        }
        catch(...)
        {
			future_object.mark_exceptional_finish_internal(boost::current_exception());
        }
	}

	template<typename T, typename F>
	static void locking_invoke(T& future_object, F& f)
	{				
        try
        {
			f();
			future_object.mark_finished_with_result();
        }
        catch(...)
        {
			future_object.mark_exceptional_finish();
        }
	}
};

template<typename R, typename F>
struct deferred_future_object : public boost::detail::future_object<R>
{	
	F f;
	bool done;

	template<typename F2>
	deferred_future_object(F2&& f)
		: f(std::forward<F2>(f))
		, done(false)
	{
		set_wait_callback(std::mem_fn(&detail::deferred_future_object<R, F>::operator()), this);
	}

	~deferred_future_object()
	{
	}
		
	void operator()()
	{		
		boost::lock_guard<boost::mutex> lock2(mutex);

		if(done)
			return;

		future_object_helper<R>::nonlocking_invoke(*this, f);

		done = true;
	}
};

template<typename R, typename F>
struct async_future_object : public boost::detail::future_object<R>
{	
	F f;
	boost::thread thread;

	template<typename F2>
	async_future_object(F2&& f)
		: f(std::forward<F2>(f))
		, thread([this]{run();})
	{
	}

	~async_future_object()
	{
		thread.join();
	}

	void run()
	{
		future_object_helper<R>::locking_invoke(*this, f);
	}
};

}
	
template<typename F>
auto async(launch policy, F&& f) -> boost::unique_future<decltype(f())>
{		
	typedef decltype(f())								result_type;	
	typedef boost::detail::future_object<result_type>	future_object_type;

	boost::shared_ptr<future_object_type> future_object;

	// HACK: This solution is a hack to avoid modifying boost code.

	if((policy & launch::async) != 0)
		future_object.reset(new detail::async_future_object<result_type, F>(std::forward<F>(f)), [](future_object_type* p){delete reinterpret_cast<detail::async_future_object<result_type, F>*>(p);});
	else if((policy & launch::deferred) != 0)
		future_object.reset(new detail::deferred_future_object<result_type, F>(std::forward<F>(f)), [](future_object_type* p){delete reinterpret_cast<detail::deferred_future_object<result_type, F>*>(p);});
	else
		throw std::invalid_argument("policy");
	
	boost::unique_future<result_type> future;

	static_assert(sizeof(future) == sizeof(future_object), "");

	reinterpret_cast<boost::shared_ptr<future_object_type>&>(future) = std::move(future_object); // Get around the "private" encapsulation.
	return std::move(future);
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

/**
 * A utility that helps the producer side of a future when the task is not
 * able to complete immediately but there are known retry points in the code.
 */
template<class R>
class retry_task
{
public:
	typedef boost::function<boost::optional<R> ()> func_type;
	
	retry_task() : done_(false) {}

	/**
	 * Reset the state with a new task. If the previous task has not completed
	 * the old one will be discarded.
	 *
	 * @param func The function that tries to calculate future result. If the
	 *             optional return value is set the future is marked as ready.
	 */
	void set_task(const func_type& func)
	{
		boost::mutex::scoped_lock lock(mutex_);

		func_ = func;
		done_ = false;
		promise_ = boost::promise<R>();
	}

	/**
	 * Take ownership of the future for the current task. Cannot only be called
	 * once for each task.
	 *
	 * @return the future.
	 */
	boost::unique_future<R> get_future()
	{
		boost::mutex::scoped_lock lock(mutex_);

		return promise_.get_future();
	}

	/**
	 * Call this when it is guaranteed or probable that the task will be able
	 * to complete.
	 *
	 * @return true if the task completed (the future will have a result).
	 */
	bool try_completion()
	{
		boost::mutex::scoped_lock lock(mutex_);

		return try_completion_internal();
	}

	/**
	 * Call this when it is certain that the result should be ready, and if not
	 * it should be regarded as an unrecoverable error (retrying again would
	 * be useless), so the future will be marked as failed.
	 *
	 * @param exception The exception to mark the future with *if* the task
	 *                  completion fails.
	 */
	void try_or_fail(const std::exception& exception)
	{
		boost::mutex::scoped_lock lock(mutex_);

		if (!try_completion_internal())
		{
			try
			{
				throw exception;
			}
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				promise_.set_exception(boost::current_exception());
				done_ = true;
			}
		}
	}
private:
	bool try_completion_internal()
	{
		if (!func_)
			return false;

		if (done_)
			return true;

		boost::optional<R> result;

		try
		{
			result = func_();
		}
		catch (...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			promise_.set_exception(boost::current_exception());
			done_ = true;

			return true;
		}

		if (result)
		{
			promise_.set_value(*result);
			done_ = true;
		}

		return done_;
	}
private:
	boost::mutex mutex_;
	func_type func_;
	boost::promise<R> promise_;
	bool done_;
};

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
boost::unique_future<R> wrap_as_future(R&& value)
{
	boost::promise<R> p;

	p.set_value(value);

	return p.get_future();
}

}