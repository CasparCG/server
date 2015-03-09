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

#include <cpplinq/linq.hpp>

#include <utility>
#include <numeric>

namespace caspar {

struct keys
{
	template <typename Pair>
	const typename Pair::first_type& operator()(const Pair& p) const
	{
		return p.first;
	}
};

struct values
{
	template <typename Pair>
	const typename Pair::second_type& operator()(const Pair& p) const
	{
		return p.second;
	}
};

struct minmax
{
	template <typename T>
	std::pair<T, T> operator()(std::pair<T, T> initial, T value) const
	{
		return std::make_pair(std::min(initial.first, value), std::max(initial.second, value));
	}

	template <typename T>
	static std::pair<T, T> initial_value()
	{
		return std::make_pair(std::numeric_limits<T>::max(), std::numeric_limits<T>::min());
	}
};

}
