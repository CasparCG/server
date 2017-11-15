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

#include <cmath>

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <map>
#include <queue>
#include <functional>

namespace caspar {

/**
 * Counting semaphore modelled after java.util.concurrent.Semaphore
 */
class semaphore : boost::noncopyable
{
	mutable boost::mutex										mutex_;
	unsigned int												permits_;
	boost::condition_variable_any								permits_available_;
	std::map<unsigned int, std::queue<std::function<void()>>>	callbacks_per_requested_permits_;
public:
	/**
	 * Constructor.
	 *
	 * @param permits The initial number of permits.
	 */
	explicit semaphore(unsigned int permits)
		: permits_(permits)
	{
	}

	/**
	 * Release a permit.
	 */
	void release()
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		++permits_;

		perform_callback_based_acquire();
		permits_available_.notify_one();
	}

	/**
	 * Release a permit.
	 *
	 * @param permits The number of permits to release.
	 */
	void release(unsigned int permits)
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		permits_ += permits;

		perform_callback_based_acquire();
		permits_available_.notify_all();
	}

	/**
	 * Acquire a permit. Will block until one becomes available if no permit is
	 * currently available.
	 */
	void acquire()
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		while (permits_ == 0u)
		{
			permits_available_.wait(lock);
		}

		--permits_;
	}

	/**
	 * Acquire a number of permits. Will block until the given number of
	 * permits has been acquired if not enough permits are currently available.
	 *
	 * @param permits The number of permits to acquire.
	 */
	void acquire(unsigned int permits)
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		auto num_acquired = 0u;

		while (true)
		{
			auto num_wanted	= permits - num_acquired;
			auto to_drain	= std::min(num_wanted, permits_);

			permits_		-= to_drain;
			num_acquired	+= to_drain;

			if (num_acquired == permits)
				break;

			permits_available_.wait(lock);
		}
	}

	/**
	* Acquire a number of permits. Will not block, but instead invoke a callback
	* when the specified number of permits are available and has been acquired.
	*
	* @param permits           The number of permits to acquire.
	* @param acquired_callback The callback to invoke when acquired.
	*/
	void acquire(unsigned int permits, std::function<void()> acquired_callback)
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		if (permits_ >= permits)
		{
			permits_ -= permits;
			lock.unlock();
			acquired_callback();
		}
		else
			callbacks_per_requested_permits_[permits].push(std::move(acquired_callback));
	}

	/**
	 * Acquire a number of permits. Will block until the given number of
	 * permits has been acquired if not enough permits are currently available
	 * or the timeout has passed.
	 *
	 * @param permits The number of permits to acquire.
	 * @param timeout The timeout (will be used for each permit).
	 *
	 * @return whether successfully acquired within timeout or not.
	 */
	template <typename Rep, typename Period>
	bool try_acquire(unsigned int permits, const boost::chrono::duration<Rep, Period>& timeout)
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		auto num_acquired = 0u;

		while (true)
		{
			auto num_wanted	= permits - num_acquired;
			auto to_drain	= std::min(num_wanted, permits_);

			permits_		-= to_drain;
			num_acquired	+= to_drain;

			if (num_acquired == permits)
				break;

			if (permits_available_.wait_for(lock, timeout) == boost::cv_status::timeout)
			{
				lock.unlock();
				release(num_acquired);
				return false;
			}
		}

		return true;
	}

	/**
	 * Acquire one permits if permits are currently available. Does not block
	 * until one is available, but returns immediately if unavailable.
	 *
	 * @return true if a permit was acquired or false if no permits where
	 *         currently available.
	 */
	bool try_acquire()
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		if (permits_ == 0u)
			return false;
		else
		{
			--permits_;

			return true;
		}
	}

	/**
	 * @return the current number of permits (may have changed at the time of
	 *         return).
	 */
	unsigned int permits() const
	{
		boost::unique_lock<boost::mutex> lock(mutex_);

		return permits_;
	}

private:
	void perform_callback_based_acquire()
	{
		if (callbacks_per_requested_permits_.empty())
			return;

		while (
			!callbacks_per_requested_permits_.empty() &&
			callbacks_per_requested_permits_.begin()->first <= permits_)
		{
			auto requested_permits_and_callbacks	= callbacks_per_requested_permits_.begin();
			auto requested_permits					= requested_permits_and_callbacks->first;
			auto& callbacks							= requested_permits_and_callbacks->second;

			if (callbacks.empty())
			{
				callbacks_per_requested_permits_.erase(requested_permits_and_callbacks);
				continue;
			}

			auto& callback							= callbacks.front();

			permits_ -= requested_permits;
			mutex_.unlock();

			try
			{
				callback();
			}
			catch (...)
			{
			}

			mutex_.lock();
			callbacks.pop();

			if (callbacks.empty())
				callbacks_per_requested_permits_.erase(requested_permits_and_callbacks);
		}
	}
};

/**
 * Enables RAII-style acquire/release on scope exit unless committed.
 */
class acquire_transaction : boost::noncopyable
{
	semaphore& semaphore_;
	bool committed_;
public:
	/**
	 * Constructor.
	 *
	 * @param semaphore        The semaphore to acquire one permit from.
	 * @param already_acquired Whether a permit has already been acquired or not.
	 */
	acquire_transaction(semaphore& semaphore, bool already_acquired = false)
		: semaphore_(semaphore)
		, committed_(false)
	{
		if (!already_acquired)
			semaphore_.acquire();
	}

	/**
	 * Destructor that will release one permit if commit() has not been called.
	 */
	~acquire_transaction()
	{
		if (!committed_)
			semaphore_.release();
	}

	/**
	 * Ensure that the acquired permit is kept on destruction.
	 */
	void commit()
	{
		committed_ = true;
	}
};

}
