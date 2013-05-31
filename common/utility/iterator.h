/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <boost/iterator/iterator_facade.hpp>

namespace caspar {

/**
 * An iterator that will automatically skip elements based on the NextFinder
 * policy.
 */
template<typename Value, typename NextFinder, typename Iter = Value*>
class position_based_skip_iterator : public boost::iterator_facade<
		position_based_skip_iterator<Value, NextFinder, Iter>,
		Value,
		boost::forward_traversal_tag>
{
	Iter pos_;
	Iter end_;
	NextFinder next_finder_;
public:
	position_based_skip_iterator()
	{
	}

	position_based_skip_iterator(const Iter& position)
		: pos_(position)
	{
	}

	position_based_skip_iterator(
			const Iter& position, const Iter& end, const NextFinder& next_finder)
		: pos_(position), end_(end), next_finder_(next_finder)
	{
	}
private:
	friend class boost::iterator_core_access;

	void increment()
	{
		pos_ = next_finder_.next(pos_, end_);
	}

	bool equal(const position_based_skip_iterator<Value, NextFinder, Iter>& other) const
	{
		return pos_ == other.pos_;
	}

	Value& dereference() const
	{
		return *pos_;
	}
};

/**
 * Simple NextFinder policy class which always steps a given distance.
 */
class constant_step_finder
{
	std::ptrdiff_t step_;
	size_t num_steps_;
	mutable size_t steps_made_;
public:
	constant_step_finder()
	{
	}

	constant_step_finder(std::ptrdiff_t step, size_t num_steps)
		: step_(step)
		, num_steps_(num_steps)
		, steps_made_(0)
	{
	}

	template<typename Iter>
	Iter next(const Iter& relative_to, const Iter& end) const
	{
		if (steps_made_ + 1 > num_steps_)
			return end;

		++steps_made_;

		return relative_to + step_;
	}
};

}
