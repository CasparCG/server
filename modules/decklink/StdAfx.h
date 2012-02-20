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
#define WIN32_LEAN_AND_MEAN

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
#include <tbb/cache_aligned_allocator.h>
#include <boost/assign.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

#include <functional>


#include "../common/memory.h"
#include "../common/utf.h"
#include "../common/except.h"
#include "../common/log.h"
