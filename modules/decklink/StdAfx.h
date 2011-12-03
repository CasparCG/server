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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#define NOMINMAX

#include <Windows.h>

#include <algorithm>
#include <array>
#include <assert.h>
#include <deque>
#include <functional>
#include <math.h>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/assign.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>

#include "../common/memory/safe_ptr.h"
#include "../common/utility/string.h"
//#include "../common/concurrency/executor.h" // Can't include this due to MSVC lambda bug

#include "../common/exception/exceptions.h"
#include "../common/exception/win32_exception.h"
#include "../common/log/Log.h"
