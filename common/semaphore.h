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
#include <boost/thread/condition.hpp>

namespace caspar {

template <class N, class Func>
void repeat_n(N times_to_repeat_block, const Func& func)
{
	for (N i = 0; i < times_to_repeat_block; ++i)
	{
		func();
	}
}

/**
 * Counting semaphore modelled after java.util.concurrent.Semaphore
 */
class semaphore : boost::noncopyable
{
	mutable boost::mutex mutex_;
	unsigned int permits_;
	boost::condition_variable permits_available_;
public:
	/**
	 * Constructor.
	 *
	 * @param permits The initial number of permits.
	 */
	semaphore(unsigned int permits)
		: permits_(permits)
	{
	}

	/**
	 * Release a permit.
	 */
	void release()
	{
		boost::mutex::scoped_lock lock(mutex_);

		++permits_;

		permits_available_.notify_one();
	}

	/**
	 * Release a permit.
	 *
	 * @param permits The number of permits to release.
	 */
	void release(unsigned int permits)
	{
		boost::mutex::scoped_lock lock(mutex_);

		permits_ += permits;

		repeat_n(permits, [this] { permits_available_.notify_one(); });
	}

	/**
	 * Acquire a permit. Will block until one becomes available if no permit is
	 * currently available.
	 */
	void acquire()
	{
		boost::mutex::scoped_lock lock(mutex_);

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
		boost::mutex::scoped_lock lock(mutex_);
		auto num_acquired = 0u;

		while (true)
		{
			auto num_wanted = permits - num_acquired;
			auto to_drain = std::min(num_wanted, permits_);

			permits_ -= to_drain;
			num_acquired += to_drain;

			if (num_acquired == permits)
				break;

			permits_available_.wait(lock);
		}
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
		boost::mutex::scoped_lock lock(mutex_);

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
		boost::mutex::scoped_lock lock(mutex_);

		return permits_;
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
