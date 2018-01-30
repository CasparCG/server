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

#include "semaphore.h"

#include <boost/noncopyable.hpp>

#include <mutex>

namespace caspar {

/**
 * Adapts an unbounded non-blocking concurrent queue into a blocking bounded
 * concurrent queue.
 *
 * The queue Q to adapt must support the following use cases:
 *
 * Q q;
 * Q::value_type elem;
 * q.push(elem);
 *
 * and:
 *
 * Q q;
 * Q::value_type elem;
 * q.try_pop(elem);
 *
 * It must also guarantee thread safety for those operations.
 */
template<class Q>
class blocking_bounded_queue_adapter : boost::noncopyable
{
public:
	typedef typename Q::value_type value_type;
	typedef unsigned int size_type;
private:
	mutable	std::mutex	    capacity_mutex_;
	size_type				capacity_;
	semaphore				space_available_		= capacity_;
	semaphore				elements_available_;
	Q						queue_;
public:
	/**
	 * Constructor.
	 *
	 * @param capacity The capacity of the queue.
	 */
	blocking_bounded_queue_adapter(size_type capacity)
		: capacity_(capacity)
	{
	}

	/**
	 * Push an element to the queue, block until room is available.
	 *
	 * @param element The element to push.
	 */
	void push(const value_type& element)
	{
		space_available_.acquire();
		push_after_room_reserved(element);
	}

	/**
	 * Try to push an element to the queue, returning immediately if room is not
	 * available.
	 *
	 * @param element The element to push.
	 *
	 * @return true if there was room for the element.
	 */
	bool try_push(const value_type& element)
	{
		bool room_available = space_available_.try_acquire();

		if (!room_available)
			return false;

		push_after_room_reserved(element);

		return true;
	}

	/**
	 * Pop an element from the queue, will block until an element is available.
	 *
	 * @param element The element to store the result in.
	 */
	void pop(value_type& element)
	{
		elements_available_.acquire();
		queue_.try_pop(element);
		space_available_.release();
	}

	/**
	 * Try to pop an element from the queue, returning immediately if no
	 * element is available.
	 *
	 * @param element The element to store the result in.
	 *
	 * @return true if an element was popped.
	 */
	bool try_pop(value_type& element)
	{
		if (!elements_available_.try_acquire())
			return false;

		queue_.try_pop(element);
		space_available_.release();

		return true;
	}

	/**
	 * Modify the capacity of the queue. May block if reducing the capacity.
	 *
	 * @param capacity The new capacity.
	 */
	void set_capacity(size_type capacity)
	{
		std::unique_lock<std::mutex> lock (capacity_mutex_);

		if (capacity_ < capacity)
		{
			auto to_grow_with = capacity - capacity_;

			space_available_.release(to_grow_with);
		}
		else if (capacity_ > capacity)
		{
			auto to_shrink_with = capacity_ - capacity;

			// Will block until the desired capacity has been reached.
			space_available_.acquire(to_shrink_with);
		}

		capacity_ = capacity;
	}

	/**
	 * @return the current capacity of the queue.
	 */
	size_type capacity() const
	{
		std::unique_lock<std::mutex> lock (capacity_mutex_);

		return capacity_;
	}

	/**
	 * @return the current size of the queue (may have changed at the time of
	 *         returning).
	 */
	size_type size() const
	{
		return elements_available_.permits();
	}
private:
	void push_after_room_reserved(const value_type& element)
	{
		try
		{
			queue_.push(element);
		}
		catch (...)
		{
			space_available_.release();

			throw;
		}

		elements_available_.release();
	}
};

}
