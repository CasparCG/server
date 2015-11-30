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

#include <asmlib.h>

#include <tbb/parallel_for.h>

#include <boost/thread/thread.hpp>

namespace caspar {

static void fast_memcpy(void* dest, const void* src, std::size_t size)
{
	tbb::affinity_partitioner partitioner;
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, size, size / boost::thread::hardware_concurrency()), [&](const tbb::blocked_range<size_t>& range)
	{
		A_memcpy(
				reinterpret_cast<std::uint8_t*>(dest) + range.begin(),
				reinterpret_cast<const std::uint8_t*>(src) + range.begin(),
				range.size());
	}, partitioner);
}

}
