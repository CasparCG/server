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

#include <string>
#include <cstdint>
#include <vector>

#include "memory.h"

#include <tbb/atomic.h>

#include <boost/thread/thread.hpp>

namespace caspar {

struct thread_info
{
	std::string		name;
	std::int64_t	native_id;

	thread_info();
};

thread_info& get_thread_info();
std::vector<spl::shared_ptr<thread_info>> get_thread_infos();

}
