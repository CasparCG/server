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

#include <stdexcept>
#include <map>
#include <initializer_list>

#include <tbb/concurrent_queue.h>

#include "semaphore.h"

namespace caspar {

/**
 * Blocking concurrent priority queue supporting a predefined set of
 * priorities. FIFO ordering is guaranteed within a priority.
 *
 * Prio must have the < and > operators defined where a larger instance is of a
 * higher priority.
 */
template <class T, class Prio>
class blocking_priority_queue
{
public:
	typedef unsigned int size_type;
private:
	std::map<Prio, tbb::concurrent_queue<T>, std::greater<Prio>>	queues_by_priority_;
	size_type														capacity_;
	semaphore														space_available_	{ capacity_ };
	semaphore														elements_available_	{ 0u };
	mutable std::mutex											    capacity_mutex_;
public:
	/**
	 * Constructor.
	 *
	 * @param capacity   The initial capacity of the queue.
	 * @param priorities A forward iterable range with the priorities to
	 *                   support.
	 */
	template<class PrioList>
	blocking_priority_queue(size_type capacity, const PrioList& priorities)
		: capacity_(capacity)
	{
		for (Prio priority : priorities)
		{
			queues_by_priority_.insert(std::make_pair(priority, tbb::concurrent_queue<T>()));
		}

		// The std::map is read-only from now on, so there *should* (it is
		// unlikely but possible for a std::map implementor to choose to
		// rebalance the tree during read operations) be no race conditions
		// regarding the map.
		//
		// This may be true for vc10 as well:
		// http://msdn.microsoft.com/en-us/library/c9ceah3b%28VS.80%29.aspx
	}

	/**
	 * Push an element with a given priority to the queue. Blocks until room
	 * is available.
	 *
	 * @param priority The priority of the element.
	 * @param element  The element.
	 */
	void push(Prio priority, const T& element)
	{
		acquire_transaction transaction(space_available_);

		push_acquired(priority, element, transaction);
	}

	/**
	 * Attempt to push an element with a given priority to the queue. Will
	 * immediately return even if there is no room in the queue.
	 *
	 * @param priority The priority of the element.
	 * @param element  The element.
	 *
	 * @return true if the element was pushed. false if there was no room.
	 */
	bool try_push(Prio priority, const T& element)
	{
		if (!space_available_.try_acquire())
			return false;

		acquire_transaction transaction(space_available_, true);

		push_acquired(priority, element, transaction);

		return true;
	}

	/**
	 * Pop the element with the highest priority (fifo for elements with the
	 * same priority). Blocks until an element is available.
	 *
	 * @param element The element to store the result in.
	 */
	void pop(T& element)
	{
		acquire_transaction transaction(elements_available_);

		pop_acquired_any_priority(element, transaction);
	}

	/**
	 * Attempt to pop the element with the highest priority (fifo for elements
	 * with the same priority) if available. Does not wait until an element is
	 * available.
	 *
	 * @param element The element to store the result in.
	 *
	 * @return true if an element was available.
	 */
	bool try_pop(T& element)
	{
		if (!elements_available_.try_acquire())
			return false;

		acquire_transaction transaction(elements_available_, true);

		pop_acquired_any_priority(element, transaction);

		return true;
	}

	/**
	 * Attempt to pop the element with the highest priority (fifo for elements
	 * with the same priority) if available *and* has a minimum priority. Does
	 * not wait until an element satisfying the priority criteria is available.
	 *
	 * @param element          The element to store the result in.
	 * @param minimum_priority The minimum priority to accept.
	 *
	 * @return true if an element was available with the minimum priority.
	 */
	bool try_pop(T& element, Prio minimum_priority)
	{
		if (!elements_available_.try_acquire())
			return false;

		acquire_transaction transaction(elements_available_, true);

		for (auto& queue : queues_by_priority_)
		{
			if (queue.first < minimum_priority)
			{
				// Will be true for all queues from this point so we break.
				break;
			}

			if (queue.second.try_pop(element))
			{
				transaction.commit();
				space_available_.release();

				return true;
			}
		}

		return false;
	}

	/**
	 * Modify the capacity of the queue. May block if reducing the capacity.
	 *
	 * @param capacity The new capacity.
	 */
	void set_capacity(size_type capacity)
	{
		std::unique_lock<std::mutex> lock(capacity_mutex_);

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

	/**
	 * @return the current available space in the queue (may have changed at
	 *         the time of returning).
	 */
	size_type space_available() const
	{
		return space_available_.permits();
	}
private:
	void push_acquired(Prio priority, const T& element, acquire_transaction& transaction)
	{
		try
		{
			queues_by_priority_.at(priority).push(element);
		}
		catch (std::out_of_range&)
		{
			throw std::runtime_error("Priority not supported by queue");
		}

		transaction.commit();
		elements_available_.release();
	}

	void pop_acquired_any_priority(T& element, acquire_transaction& transaction)
	{
		for (auto& queue : queues_by_priority_)
		{
			if (queue.second.try_pop(element))
			{
				transaction.commit();
				space_available_.release();

				return;
			}
		}

		throw std::logic_error(
				"blocking_priority_queue should have contained at least one element but didn't");
	}
};

}
