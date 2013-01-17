/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Helge Norberg, helge.norberg@svt.se
*/
#pragma once

#include "../log/log.h"

#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>

namespace caspar
{

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
	template <class E>
	void try_or_fail(const E& exception)
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
