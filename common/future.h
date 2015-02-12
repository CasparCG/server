#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <future>

namespace caspar {

template<typename T>
auto flatten(std::future<T>&& f) -> std::future<typename std::decay<decltype(f.get().get())>::type>
{
	auto shared_f = f.share();
	return std::async(std::launch::deferred, [=]() mutable -> typename std::decay<decltype(f.get().get())>::type
	{
		return shared_f.get().get();
	});
}

template<typename F>
bool is_ready(const F& future)
{
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
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
		boost::unique_lock<boost::mutex> lock(mutex_);

		func_ = func;
		done_ = false;
		promise_ = std::promise<R>();
	}

	/**
	 * Take ownership of the future for the current task. Cannot only be called
	 * once for each task.
	 *
	 * @return the future.
	 */
	std::future<R> get_future()
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

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
		boost::unique_lock<boost::mutex> lock(mutex_);

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
	template <class E>
	void try_or_fail(const E& exception)
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		if (!try_completion_internal())
		{
			try
			{
				throw exception;
			}
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				promise_.set_exception(std::current_exception());
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
			promise_.set_exception(std::current_exception());
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
	std::promise<R> promise_;
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

}